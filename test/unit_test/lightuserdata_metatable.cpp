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

struct Foo {
  Foo(int i) : v(i) {}
  int v;
};

TEST(lightuserdata_metatable, set_addr_like_number) {
  luaw l;
  l.register_member("v", &Foo::v);

  Foo a(1);
  EXPECT_EQ(a.v, 1);

  l.set<void*>("a", (void*)(&a));

  {
    bool failed;
    int  v = l.eval_int("return a.v", -1, false, &failed);
    EXPECT_TRUE(failed);
    EXPECT_EQ(v, -1);
  }

  // After set mebtatable
  l.set_lightuserdata_metatable<Foo*>();
  {
    bool failed;
    int  v = l.eval_int("return a.v", -1, false, &failed);
    EXPECT_FALSE(failed);
    EXPECT_EQ(v, 1);
  }

  // After clear metatable
  l.clear_lightuserdata_metatable();
  {
    bool failed;
    int  v = l.eval_int("return a.v", -1, false, &failed);
    EXPECT_TRUE(failed);
    EXPECT_EQ(v, -1);
  }
}

TEST(lightuserdata_metatable, set_pointer_to_class) {
  luaw l;
  l.register_member("v", &Foo::v);

  Foo a(1);
  EXPECT_EQ(a.v, 1);

  l.set("a", &a);
  {
    bool failed;
    int  v = l.eval_int("return a.v", -1, false, &failed);
    EXPECT_FALSE(failed);
    EXPECT_EQ(v, 1);
  }

  // After clear metatable
  l.clear_lightuserdata_metatable();
  {
    bool failed;
    int  v = l.eval_int("return a.v", -1, false, &failed);
    EXPECT_TRUE(failed);
    EXPECT_EQ(v, -1);
  }
}

struct Bar {
  const int cv = 123;
};

TEST(lightuserdata_metatable, two_types) {
  luaw l;
  l.register_member("v", &Foo::v);

  Foo a(1);
  Bar b;
  EXPECT_EQ(a.v, 1);
  EXPECT_EQ(b.cv, 123);

  l.set("b", &b);
  std::string metatablename1 =
      l.lget<std::string>({}, "b", peacalm::luaw::metatable_tag{}, "__name");

  EXPECT_EQ(l.eval_int("return b.v"), 0);

  l.set("a", &a);
  std::string metatablename2 =
      l.lget<std::string>({}, "b", peacalm::luaw::metatable_tag{}, "__name");

  EXPECT_NE(metatablename1, metatablename2);

  // Behavior with wrong metatable is undefined,
  // we should not make any assurance
#if 0
  EXPECT_EQ(l.eval_int("return a.v"), 1);
  EXPECT_EQ(l.eval_int("return b.v"), 123);
  EXPECT_EQ(l.eval_int("return a.cv"), 0);
  EXPECT_EQ(l.eval_int("return b.cv"), 0);

  l.register_member("cv", &Bar::cv);
  l.set_lightuserdata_metatable<const Bar*>();
  EXPECT_EQ(l.eval_int("return a.v"), 0);
  EXPECT_EQ(l.eval_int("return b.v"), 0);
  EXPECT_EQ(l.eval_int("return a.cv"), 1);
  EXPECT_EQ(l.eval_int("return b.cv"), 123);
#endif

  // metatable for Bar* and const Bar* is different
  l.set_lightuserdata_metatable<const Bar*>();
  std::string metatablename3 =
      l.lget<std::string>({}, "b", peacalm::luaw::metatable_tag{}, "__name");
  EXPECT_NE(metatablename3, metatablename1);

  l.set_lightuserdata_metatable<Bar*>();
  std::string metatablename4 =
      l.lget<std::string>({}, "b", peacalm::luaw::metatable_tag{}, "__name");
  EXPECT_EQ(metatablename4, metatablename1);
}
