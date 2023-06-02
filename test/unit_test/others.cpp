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

#include "main.h"

TEST(others, abs_index) {
  lua_wrapper l;
  EXPECT_EQ(l.gettop(), 0);
  EXPECT_EQ(l.abs_index(0), 0);
  EXPECT_EQ(l.abs_index(1), 1);
  EXPECT_EQ(l.abs_index(2), 2);
  EXPECT_EQ(l.abs_index(5), 5);
  EXPECT_EQ(l.abs_index(-1), -1);
  EXPECT_EQ(l.abs_index(-2), -2);
  EXPECT_EQ(l.abs_index(-5), -5);

  l.settop(3);
  EXPECT_EQ(l.abs_index(0), 0);
  EXPECT_EQ(l.abs_index(1), 1);
  EXPECT_EQ(l.abs_index(2), 2);
  EXPECT_EQ(l.abs_index(-1), 3);
  EXPECT_EQ(l.abs_index(-2), 2);
  EXPECT_EQ(l.abs_index(-3), 1);
  EXPECT_EQ(l.abs_index(-4), -4);
  EXPECT_EQ(l.abs_index(-5), -5);
}

TEST(others, indexable) {
  lua_wrapper l;
  EXPECT_EQ(l.gettop(), 0);
  l.dostring("t={} a=1 ");
  EXPECT_EQ(l.gettop(), 0);
  l.gseek("t");

  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.indexable());
  EXPECT_TRUE(l.newindexable());
  EXPECT_TRUE(l.indexable_and_newindexable());
  EXPECT_FALSE(l.callable());
  EXPECT_EQ(l.gettop(), 1);

  l.cleartop();
  l.gseek("a");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_FALSE(l.indexable());
  EXPECT_FALSE(l.newindexable());
  EXPECT_FALSE(l.indexable_and_newindexable());
  EXPECT_FALSE(l.callable());
  EXPECT_EQ(l.gettop(), 1);

  l.cleartop();
  l.dostring("f = function(x, y) return x + y end ");
  l.gseek("f");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_FALSE(l.indexable());
  EXPECT_FALSE(l.newindexable());
  EXPECT_FALSE(l.indexable_and_newindexable());
  EXPECT_TRUE(l.callable());
  EXPECT_EQ(l.gettop(), 1);

  l.cleartop();
  l.push([](int a, int b) { return a + b; });
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_FALSE(l.indexable());
  EXPECT_FALSE(l.newindexable());
  EXPECT_FALSE(l.indexable_and_newindexable());
  EXPECT_TRUE(l.callable());
  EXPECT_EQ(l.gettop(), 1);

  l.cleartop();
  l.gseek("a");
  EXPECT_FALSE(l.istable(1));
  l.touchtb(lua_wrapper::metatable_tag{});
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_FALSE(l.indexable(1));
  EXPECT_FALSE(l.newindexable(1));
  EXPECT_FALSE(l.indexable_and_newindexable(1));
  EXPECT_FALSE(l.callable(1));

  EXPECT_TRUE(l.istable(2));
  EXPECT_TRUE(l.indexable(2));
  EXPECT_TRUE(l.newindexable(2));
  EXPECT_TRUE(l.indexable_and_newindexable(2));
  EXPECT_FALSE(l.callable(2));

  EXPECT_EQ(l.gettop(), 2);
  l.setfield("__index", []() { return 1; });
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.indexable(1));
  EXPECT_FALSE(l.newindexable(1));
  EXPECT_FALSE(l.indexable_and_newindexable(1));
  EXPECT_FALSE(l.callable(1));
  EXPECT_EQ(l.gettop(), 2);

  //
  l.setfield("__newindex", []() { return 1; });
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.indexable(1));
  EXPECT_TRUE(l.newindexable(1));
  EXPECT_TRUE(l.indexable_and_newindexable(1));
  EXPECT_FALSE(l.callable(1));
  EXPECT_EQ(l.gettop(), 2);

  //
  l.setfield("__call", []() { return 1; });
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.indexable(1));
  EXPECT_TRUE(l.newindexable(1));
  EXPECT_TRUE(l.indexable_and_newindexable(1));
  EXPECT_TRUE(l.callable(1));
  EXPECT_EQ(l.gettop(), 2);
}
