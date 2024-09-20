// Copyright (c) 2024 Li Shuangquan. All Rights Reserved.
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

#include "main.h"

namespace {

struct Obj {
  static int       si;
  static const int sci;
  static int       sqr(int x) { return x * x; }

  // No support C variadic function
  static int pf(const char* s, ...) { return 0; }
};
int       Obj::si  = 1;
const int Obj::sci = 1;

TEST(register_static_member, variable_by_class) {
  Obj::si = 1;
  luaw l;
  l.register_static_member<Obj>("si", &Obj::si);

  Obj o;
  l.set("o", o);
  EXPECT_EQ(l.eval<int>("return o.si"), 1);
  Obj::si = 2;
  EXPECT_EQ(l.eval<int>("return o.si"), 2);

  int retcode = l.dostring("o.si = 3");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.si"), 3);
  EXPECT_EQ(Obj::si, 3);

  // const object
  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c.si"), 3);
  EXPECT_TRUE(l.eval<bool>("return o.si == c.si"));

  // Can not modify static member by const object in Lua
  retcode = l.dostring("c.si = 4");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.si"), 3);
  EXPECT_EQ(Obj::si, 3);

  // But can modify it in C++ by const object
  c.si = 5;
  EXPECT_EQ(l.eval<int>("return c.si"), 5);

  // Share same si by different objects in Lua
  retcode = l.dostring("o.si = 6");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.si"), 6);
  EXPECT_EQ(l.eval<int>("return c.si"), 6);
  EXPECT_TRUE(l.eval<bool>("return o.si == c.si"));
  auto s = std::make_shared<Obj>();
  l.set("s", s);
  EXPECT_EQ(l.eval<int>("return s.si"), 6);
  EXPECT_TRUE(l.eval<bool>("return s.si == o.si"));
}

TEST(register_static_member, variable_by_full_type) {
  Obj::si = 1;
  luaw l;
  l.register_static_member<int Obj::*>("si", &Obj::si);

  Obj o;
  l.set("o", o);
  EXPECT_EQ(l.eval<int>("return o.si"), 1);
  Obj::si = 2;
  EXPECT_EQ(l.eval<int>("return o.si"), 2);

  int retcode = l.dostring("o.si = 3");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.si"), 3);
  EXPECT_EQ(Obj::si, 3);

  // const object
  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c.si"), 3);
  EXPECT_TRUE(l.eval<bool>("return o.si == c.si"));

  // Can not modify static member by const object in Lua
  retcode = l.dostring("c.si = 4");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.si"), 3);
  EXPECT_EQ(Obj::si, 3);

  // But can modify it in C++ by const object
  c.si = 5;
  EXPECT_EQ(l.eval<int>("return c.si"), 5);

  // Share same si by different objects in Lua
  retcode = l.dostring("o.si = 6");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.si"), 6);
  EXPECT_EQ(l.eval<int>("return c.si"), 6);
  EXPECT_TRUE(l.eval<bool>("return o.si == c.si"));
  auto s = std::make_shared<Obj>();
  l.set("s", s);
  EXPECT_EQ(l.eval<int>("return s.si"), 6);
  EXPECT_TRUE(l.eval<bool>("return s.si == o.si"));
}

TEST(register_static_member, const_variable_by_class) {
  Obj::si = 1;
  luaw l;
  l.register_static_member<Obj>("sci", &Obj::sci);

  EXPECT_EQ(Obj::sci, 1);
  Obj o;
  l.set("o", o);
  EXPECT_EQ(l.eval<int>("return o.sci"), 1);

  int retcode = l.dostring("o.sci = 3");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.sci"), 1);
  EXPECT_EQ(Obj::sci, 1);

  // const object
  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c.sci"), 1);
  EXPECT_TRUE(l.eval<bool>("return o.sci == c.sci"));

  retcode = l.dostring("c.sci = 4");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.sci"), 1);
  EXPECT_EQ(Obj::sci, 1);

  auto s = std::make_shared<Obj>();
  l.set("s", s);
  EXPECT_EQ(l.eval<int>("return s.sci"), 1);
  EXPECT_TRUE(l.eval<bool>("return s.sci == o.sci"));
}

TEST(register_static_member, variable_add_const_by_full_type) {
  Obj::si = 1;
  luaw l;
  // register si as const
  l.register_static_member<const int Obj::*>("si", &Obj::si);

  Obj o;
  l.set("o", o);
  EXPECT_EQ(l.eval<int>("return o.si"), 1);

  // Can not modify si in Lua
  int retcode = l.dostring("o.si = 2");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.si"), 1);
  EXPECT_EQ(Obj::si, 1);

  // Can modify in C++
  Obj::si = 3;
  EXPECT_EQ(l.eval<int>("return o.si"), 3);

  // const object
  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c.si"), 3);
  EXPECT_TRUE(l.eval<bool>("return c.si == o.si"));

  retcode = l.dostring("c.si = 4");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.si"), 3);
  EXPECT_EQ(Obj::si, 3);
}

TEST(register_static_member, const_variable_add_const_by_full_type) {
  Obj::si = 1;
  luaw l;
  l.register_static_member<const int Obj::*>("sci", &Obj::sci);

  l.set("o", Obj{});
  EXPECT_EQ(l.eval<int>("return o.sci"), 1);

  int retcode = l.dostring("o.sci = 3");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.sci"), 1);
}

TEST(register_static_member, function_by_class) {
  luaw l;
  l.register_static_member<Obj>("sqr", &Obj::sqr);

  l.set("o", Obj{});
  EXPECT_EQ(l.eval<int>("return o:sqr(2)"), 4);

  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c:sqr(3)"), 9);

  l.set("s", std::make_shared<Obj>());
  EXPECT_EQ(l.eval<int>("return s:sqr(4)"), 16);
  l.set("sc", std::make_shared<const Obj>());
  EXPECT_EQ(l.eval<int>("return sc:sqr(4)"), 16);
}

TEST(register_static_member, function_direct_by_class) {
  luaw l;
  l.register_static_member<Obj>("sqr", Obj::sqr);

  l.set("o", Obj{});
  EXPECT_EQ(l.eval<int>("return o:sqr(2)"), 4);

  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c:sqr(3)"), 9);

  l.set("s", std::make_shared<Obj>());
  EXPECT_EQ(l.eval<int>("return s:sqr(4)"), 16);
  l.set("sc", std::make_shared<const Obj>());
  EXPECT_EQ(l.eval<int>("return sc:sqr(4)"), 16);
}

TEST(register_static_member, function_by_full_type) {
  luaw l;
  l.register_static_member<int (Obj::*)(int) const>("sqr", &Obj::sqr);

  l.set("o", Obj{});
  EXPECT_EQ(l.eval<int>("return o:sqr(2)"), 4);

  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c:sqr(3)"), 9);

  l.set("s", std::make_shared<Obj>());
  EXPECT_EQ(l.eval<int>("return s:sqr(4)"), 16);
  l.set("sc", std::make_shared<const Obj>());
  EXPECT_EQ(l.eval<int>("return sc:sqr(4)"), 16);
}

TEST(register_static_member, function_nonconst_by_full_type) {
  luaw l;
  l.register_static_member<int (Obj::*)(int)>("sqr", &Obj::sqr);

  l.set("o", Obj{});
  EXPECT_EQ(l.eval<int>("return o:sqr(2)"), 4);

  // const object c can't call non-const member function sqr
  const Obj c;
  l.set("c", c);
  int retcode = l.dostring("t = c:sqr(3)");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval_int("return c:sqr(3)", -1), -1);

  l.set("d", std::add_const_t<Obj>{});
  retcode = l.dostring("t = d:sqr(3)");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval_int("return d:sqr(3)", -1), -1);

  l.set("s", std::make_shared<Obj>());
  EXPECT_EQ(l.eval<int>("return s:sqr(4)"), 16);

  l.set("sc", std::make_shared<const Obj>());
  EXPECT_EQ(l.eval_int("return sc:sqr(4)", -1), -1);
}

int g = 1;
int fmul(int a, int b) { return a * b; }

TEST(register_static_member, fake_static_by_global_variable) {
  luaw l;

  l.register_static_member<Obj>("g", &g);

  Obj o;
  l.set("o", o);
  EXPECT_EQ(l.eval<int>("return o.g"), 1);
  g = 2;
  EXPECT_EQ(l.eval<int>("return o.g"), 2);

  int retcode = l.dostring("o.g = 3");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.g"), 3);
  EXPECT_EQ(g, 3);

  // const object
  const Obj c;
  l.set("c", c);
  EXPECT_EQ(l.eval<int>("return c.g"), 3);
  EXPECT_TRUE(l.eval<bool>("return o.g == c.g"));

  // Can not modify static member by const object in Lua
  retcode = l.dostring("c.g = 4");
  EXPECT_NE(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.g"), 3);
  EXPECT_EQ(g, 3);

  // Share same g by different objects in Lua
  retcode = l.dostring("o.g = 6");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.eval<int>("return o.g"), 6);
  EXPECT_EQ(l.eval<int>("return c.g"), 6);
  EXPECT_TRUE(l.eval<bool>("return o.g == c.g"));
  auto s = std::make_shared<Obj>();
  l.set("s", s);
  EXPECT_EQ(l.eval<int>("return s.g"), 6);
  EXPECT_TRUE(l.eval<bool>("return s.g == o.g"));
}

TEST(register_static_member, fake_static_by_global_function) {
  luaw l;
  l.register_static_member<Obj>("fmul", &fmul);

  l.set("o", Obj{});
  EXPECT_EQ(l.eval_int("return o:fmul(2,3)", -1), 6);

  l.set("c", std::add_const_t<Obj>{});
  EXPECT_EQ(l.eval_int("return c:fmul(3,5)", -1), 15);

  const Obj d;
  l.set("d", d);
  EXPECT_EQ(l.eval_int("return d:fmul(3,5)", -1), 15);
}

}  // namespace
