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

TEST(get_metatable_name, nested_class_by_member) {
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

  watch(cm, bm, am, cbm, cbam);

  EXPECT_EQ(l.gettop(), 0);

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

  EXPECT_EQ(l.gettop(), 0);
}

TEST(get_metatable_name, nested_class_by_member_ptr_ref) {
  luaw l;
  l.register_member("b", &C::b);
  l.register_member("a", &B::a);
  l.register_member("i", &A::i);

  l.register_member_ptr("bptr", &C::b);
  l.register_member_ref("bref", &C::b);
  l.register_member_ptr("aptr", &B::a);
  l.register_member_ref("aref", &B::a);

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

  watch(cm, bm, am, cbm, cbam);

  auto cbptram = l.get_metatable_name({"c", "bptr", "a"});
  auto cbrefam = l.get_metatable_name({"c", "bref", "a"});

  EXPECT_EQ(cbptram, cbrefam);
  EXPECT_EQ(cbrefam, am);

  watch(cbptram, cbrefam);

  auto cbaptrm    = l.get_metatable_name({"c", "b", "aptr"});
  auto cbptraptrm = l.get_metatable_name({"c", "bptr", "aptr"});
  auto cbrefaptrm = l.get_metatable_name({"c", "bref", "aptr"});
  EXPECT_EQ(cbaptrm, cbptraptrm);
  EXPECT_EQ(cbaptrm, cbrefaptrm);

  watch(cbaptrm, cbptraptrm, cbrefaptrm);

  auto cbarefm    = l.get_metatable_name({"c", "b", "aref"});
  auto cbptrarefm = l.get_metatable_name({"c", "bptr", "aref"});
  auto cbrefarefm = l.get_metatable_name({"c", "bref", "aref"});
  EXPECT_EQ(cbarefm, cbptrarefm);
  EXPECT_EQ(cbarefm, cbrefarefm);

  watch(cbarefm, cbptrarefm, cbrefarefm);

  EXPECT_EQ(l.gettop(), 0);

  {
    B b;
    l.set_ptr_by_wrapper("bw", &b);
    auto bwm = l.get_metatable_name("bw");

    auto cbrefm = l.get_metatable_name({"c", "bref"});
    EXPECT_EQ(bwm, cbrefm);

    watch(bwm, cbrefm);
  }

  EXPECT_EQ(l.gettop(), 0);
}

}  // namespace
