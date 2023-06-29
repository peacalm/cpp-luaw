// Copyright (c) 2023 Shuangquan Li. All Rights Reserved.
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
  int       i  = 1;
  const int ci = 1;

  Obj() {}
  Obj(int v, int cv = 1) : i(v), ci(cv) {}

  int abs() const { return std::abs(i); }

  int plus() { return ++i; }
  int plus(int d) {
    i += d;
    return i;
  }
};

int main() {
  peacalm::luaw l;

  l.register_ctor<Obj()>("NewObj");           // default constructor
  l.register_ctor<Obj(int)>("NewObj1");       // constructor with 1 argument
  l.register_ctor<Obj(int, int)>("NewObj2");  // constructor with 2 argument

  // the constructors will generate a const instance of Obj in Lua
  using ConstObj = const Obj;
  l.register_ctor<ConstObj()>("NewConstObj");  // default constructor
  l.register_ctor<ConstObj(int)>(
      "NewConstObj1");  // constructor with 1 argument
  l.register_ctor<ConstObj(int, int)>(
      "NewConstObj2");  // constructor with 2 argument

  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);  // const member
  l.register_member("abs", &Obj::abs);

  // Register overloaded member functions.
  l.register_member<int (Obj::*)()>("plus", &Obj::plus);
  l.register_member<int (Obj::*)(int)>("plusby", &Obj::plus);

  int retcode =
      l.dostring("o = NewObj(); o.i = -100; o:plus(); absi = o:abs();");
  if (retcode != LUA_OK) {
    l.log_error_out();
    return 1;
  }
  int absi = l.get_int("absi");
  Obj o    = l.get<Obj>("o");

  std::cout << "absi = " << absi << std::endl;
  std::cout << "o.i = " << o.i << std::endl;
  std::cout << "o.ci = " << o.ci << std::endl;

  return 0;
}