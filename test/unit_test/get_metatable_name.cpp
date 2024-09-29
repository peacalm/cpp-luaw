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

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

// Struct definitions for volatile.
// Make the operator= return void to mute GNU compilation warning:
// implicit dereference will not access object of type ‘volatile xxx’ ...

struct A {
  int i;

  A() : i(1) {}
  A(const volatile A& r) : i(r.i) {}
  A(const volatile A&& r) : i(r.i) {}

  void operator=(const volatile A& r) volatile { i = r.i; }
  void operator=(const volatile A&& r) volatile { i = r.i; }
};

struct B {
  int i;
  A   a;

  B() : i(2) {}
  B(const volatile B& r) : i(r.i), a(r.a) {}
  B(const volatile B&& r) : i(r.i), a(std::move(r.a)) {}

  void operator=(const volatile B& r) volatile {
    i = r.i;
    a = r.a;
  }
  void operator=(const volatile B&& r) volatile {
    i = r.i;
    a = r.a;
  }
};

#else

struct A {
  int i = 1;
};

struct B {
  int i = 2;
  A   a;
};
#endif

struct C {
  B b;
};

TEST(get_metatable_name, nested_class) {
  luaw l;
  l.register_member("b", &C::b);
  l.register_member("a", &B::a);
  l.register_member("i", &A::i);

  l.set("c", C{});
  l.set("b", B{});
  l.set("a", A{});

  auto cm   = l.get_metatable_name("c");
  auto bm   = l.get_metatable_name("b");
  auto am   = l.get_metatable_name("a");
  auto cbm  = l.get_metatable_name({"c", "b"});
  auto cbam = l.get_metatable_name({"c", "b", "a"});

  EXPECT_EQ(bm, cbm);
  EXPECT_EQ(am, cbam);

  {
    auto _b = l.get_metatable_name(std::vector<const char*>{"c", "b"});
    EXPECT_EQ(_b, bm);
  }
  {
    auto _b = l.get_metatable_name(std::vector<std::string>{"c", "b"});
    EXPECT_EQ(_b, bm);
  }
  {
    auto _b = l.get_metatable_name({std::string("c"), std::string("b")});
    EXPECT_EQ(_b, bm);
  }

  {
    auto _a = l.get_metatable_name(std::vector<const char*>{"c", "b", "a"});
    EXPECT_EQ(_a, am);
  }
  {
    auto _a = l.get_metatable_name(std::vector<std::string>{"c", "b", "a"});
    EXPECT_EQ(_a, am);
  }
  {
    auto _a = l.get_metatable_name(
        {std::string("c"), std::string("b"), std::string("a")});
    EXPECT_EQ(_a, am);
  }
}

}  // namespace
