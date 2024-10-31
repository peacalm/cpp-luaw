// Copyright (c) 2023-2024 Li Shuangquan. All Rights Reserved.
//
// Licensed under the MIT License (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License
// at
//
//   http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.

#include <peacalm/luaw.h>

struct Obj {
  Obj() {}
  Obj(int v) : i(v) {}
  Obj(int v, int cv) : i(v), ci(cv) {}

  int abs() const { return std::abs(i); }

  int plus() { return ++i; }
  int plus(int d) {
    i += d;
    return i;
  }

  static int sqr(int x) { return x * x; }

  int              i  = 1;
  const int        ci = 1;
  static int       si;
  static const int sci;
};
int       Obj::si  = 1;
const int Obj::sci = 1;

int main() {
  peacalm::luaw l;

  // Default constructors
  l.register_ctor<Obj()>("NewObj");           // default ctor
  l.register_ctor<Obj(int)>("NewObj1");       // ctor with 1 argument
  l.register_ctor<Obj(int, int)>("NewObj2");  // ctor with 2 arguments

  // These constructors will generate a const instance of Obj in Lua
  using ConstObj = const Obj;
  l.register_ctor<ConstObj()>("NewConstObj");           // default ctor
  l.register_ctor<ConstObj(int)>("NewConstObj1");       // ctor with 1 argument
  l.register_ctor<ConstObj(int, int)>("NewConstObj2");  // ctor with 2 arguments

  // Register members
  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);  // const member

  // Register member functions
  l.register_member("abs", &Obj::abs);

  // Register overloaded member functions
  l.register_member<int (Obj::*)()>("plus", &Obj::plus);
  l.register_member<int (Obj::*)(int)>("plusby", &Obj::plus);

  // Register static members
  l.register_static_member<Obj>("si", &Obj::si);
  l.register_static_member<Obj>("sci", &Obj::sci);

  // Register static member function
  l.register_static_member<Obj>("sqr", &Obj::sqr);

  // Register a fake member "id", which is the object's address
  l.register_member<void* const(Obj::*)>(
      "id", [](const volatile Obj* p) { return (void*)p; });

  int retcode = l.dostring(
      "o = NewObj(); o.i = -100; o:plus(); absi = o:abs(); o.si = 2;");
  if (retcode != LUA_OK) {
    l.log_error_out();
    return 1;
  }
  assert(retcode == LUA_OK);

  int absi = l.get_int("absi");
  Obj o    = l.get<Obj>("o");
  assert(absi == 99);
  assert(o.i == -99);
  assert(o.ci == 1);
  assert(o.si == 2);
  assert(Obj::si == 2);  // static member is shared

  // Const member can't be modified
  retcode = l.dostring("o.ci = 2");
  assert(retcode != LUA_OK);
  l.log_error_out();
  // Lua: [string "o.ci = 2"]:1: Const member cannot be modified: ci
  assert(o.ci == 1);

  // Static const member can't be modified
  retcode = l.dostring("o.sci = 2");
  assert(retcode != LUA_OK);
  l.log_error_out();
  // Lua: [string "o.sci = 2"]:1: Const member cannot be modified: sci

  // Call static member function
  assert(l.eval_int("return o:sqr(-3)") == 9);

  // Set a shared instance to Lua using shared_ptr
  auto o2 = std::make_shared<Obj>();
  l.set("o2", o2);  // set by copy
  l.dostring("o2:plusby(100)");
  assert(l.eval_int("return o2.i") == 101);
  assert(l.get_int({"o2", "i"}) == 101);
  assert(o2->i == 101);

  o2->i = 22;
  assert(l.eval_int("return o2.i") == 22);

  // Fake member "id"
  assert(l.eval<void*>("return o2.id") == (void*)(o2.get()));
  l.set("o3", o2);
  assert(l.dostring("assert(o2.id == o3.id)") == LUA_OK);
  assert(l.dostring("assert(o2.i == o3.i)") == LUA_OK);

  // Example for const object instance
  retcode = l.dostring("co = NewConstObj2(-1, -2)");
  assert(retcode == LUA_OK);
  assert(l.eval_int("return co:abs()") == 1);
  assert(l.eval_int("return co.i") == -1);
  assert(l.eval_int("return co.ci") == -2);
  assert(l.eval_int("return co.sci") == 1);

  retcode = l.dostring("co:plus()");
  assert(retcode != LUA_OK);
  l.log_error_out();
  // Lua: [string "co:plus()"]:1: Nonconst member function: plus

  bool failed = false;
  int  coi    = l.eval_int("co.i = 1; return co.i",
                       /* def */ 0,
                       /* disable_log */ false,
                       &failed);
  // Lua: [string "co.i = 1; return co.i"]:1: Const member cannot be modified: i

  assert(coi == 0);
  assert(failed);

  return 0;
}
