// Copyright (c) 2023 Li Shuangquan. All Rights Reserved.
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

struct A {
  A() { ++ctor; }
  A(const A&) { ++ctor; }
  A(A&&) { ++ctor; }
  virtual ~A() { ++dtor; }

  static void reset() {
    ctor = 0;
    dtor = 0;
  }
  static std::atomic<int> ctor, dtor;
};
std::atomic<int> A::ctor{0};
std::atomic<int> A::dtor{0};

struct B {};

TEST(object_destruction, raw_object) {
  {
    {
      A a;
      EXPECT_EQ(A::ctor, 1);
    }
    EXPECT_EQ(A::dtor, 1);
  }

  A::reset();
  EXPECT_EQ(A::ctor, 0);
  EXPECT_EQ(A::dtor, 0);

  {
    {
      A a;
      EXPECT_EQ(A::ctor, 1);
      EXPECT_EQ(A::dtor, 0);
      luaw l;
      l.set("a", a);
      EXPECT_EQ(A::ctor, 2);
      EXPECT_EQ(A::dtor, 0);
      l.close();
      EXPECT_EQ(A::ctor, 2);
      EXPECT_EQ(A::dtor, 1);
    }
    EXPECT_EQ(A::dtor, 2);
  }

  A::reset();
  EXPECT_EQ(A::ctor, 0);
  EXPECT_EQ(A::dtor, 0);

  {
    {
      A a;
      EXPECT_EQ(A::ctor, 1);
      EXPECT_EQ(A::dtor, 0);
      luaw l;
      l.set("a", &a);
      EXPECT_EQ(A::ctor, 1);
      EXPECT_EQ(A::dtor, 0);
    }
    EXPECT_EQ(A::dtor, 1);
  }

  A::reset();
  EXPECT_EQ(A::ctor, 0);
  EXPECT_EQ(A::dtor, 0);

  {
    luaw l;
    l.register_ctor<A()>("NewA");
    int retcode = l.dostring("a = NewA()");
    EXPECT_EQ(retcode, LUA_OK);
    l.close();
    EXPECT_EQ(A::ctor, A::dtor);
  }

  A::reset();
  EXPECT_EQ(A::ctor, 0);
  EXPECT_EQ(A::dtor, 0);
}

TEST(object_destruction, shared_ptr) {
  {
    luaw l;
    auto s = std::make_shared<B>();
    EXPECT_EQ(s.use_count(), 1);

    l.set("s", s);  // by copy
    EXPECT_EQ(s.use_count(), 2);

    int retcode = l.dostring("ss = s");
    EXPECT_EQ(retcode, LUA_OK);
    EXPECT_EQ(s.use_count(), 2);

    l.close();
    EXPECT_EQ(s.use_count(), 1);

    s = nullptr;
    EXPECT_EQ(s.use_count(), 0);
  }

  A::reset();
  EXPECT_EQ(A::ctor, 0);
  EXPECT_EQ(A::dtor, 0);

  {
    luaw l;
    auto s = std::make_shared<A>();
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.set("s", s);
    EXPECT_EQ(s.use_count(), 2);
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    int retcode = l.dostring("ss = s");
    EXPECT_EQ(retcode, LUA_OK);
    EXPECT_EQ(s.use_count(), 2);
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.set("s2", s);
    EXPECT_EQ(s.use_count(), 3);
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.set("sp", &s);
    EXPECT_EQ(s.use_count(), 3);
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    s.reset();
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.close();
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 1);

    A::reset();
  }
}

TEST(object_destruction, unique_ptr) {
  A::reset();
  EXPECT_EQ(A::ctor, 0);
  EXPECT_EQ(A::dtor, 0);

  {
    luaw l;
    auto u = std::make_unique<A>();
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.set("up", &u);
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.set("u", std::move(u));
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);
    EXPECT_FALSE(u);

    int retcode = l.dostring("uu = u");
    EXPECT_EQ(retcode, LUA_OK);
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 0);

    l.close();
    EXPECT_EQ(A::ctor, 1);
    EXPECT_EQ(A::dtor, 1);

    A::reset();
  }
}
