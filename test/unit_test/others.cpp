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
  luaw l;
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

  // LUA_REGISTRYINDEX won't change
  EXPECT_EQ(l.abs_index(LUA_REGISTRYINDEX), LUA_REGISTRYINDEX);
}

TEST(others, indexable) {
  luaw l;
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
  l.touchtb(luaw::metatable_tag{});
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
  l.setkv("__index", []() { return 1; });
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.indexable(1));
  EXPECT_FALSE(l.newindexable(1));
  EXPECT_FALSE(l.indexable_and_newindexable(1));
  EXPECT_FALSE(l.callable(1));
  EXPECT_EQ(l.gettop(), 2);

  //
  l.setkv("__newindex", []() { return 1; });
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.indexable(1));
  EXPECT_TRUE(l.newindexable(1));
  EXPECT_TRUE(l.indexable_and_newindexable(1));
  EXPECT_FALSE(l.callable(1));
  EXPECT_EQ(l.gettop(), 2);

  //
  l.setkv("__call", []() { return 1; });
  EXPECT_EQ(l.gettop(), 2);

  EXPECT_FALSE(l.istable(1));
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.indexable(1));
  EXPECT_TRUE(l.newindexable(1));
  EXPECT_TRUE(l.indexable_and_newindexable(1));
  EXPECT_TRUE(l.callable(1));
  EXPECT_EQ(l.gettop(), 2);
}

TEST(others, callf) {
  luaw l;

  l.dostring("f1 = function(a, b) return a + b end");
  EXPECT_EQ(l.callf<int>("f1", 1, 1), 2);
  EXPECT_EQ(l.callf<int>("f1", 1.5, 1.5), 3);
  EXPECT_EQ((l.callf<int, double, double>("f1", 1.5, 1.5)), 3);

  l.dostring("f2 = function(a, b) return a + b, a - b end");
  EXPECT_EQ(l.callf<int>("f2", 1, 1), 2);
  EXPECT_EQ((l.callf<std::tuple<int, int>>("f2", 1, 1)),
            (std::tuple<int, int>(2, 0)));

  //
  l.dostring("g ={f1=f1, f2=f2}");

  EXPECT_EQ(l.callf<int>({"g", "f1"}, 1, 1), 2);
  EXPECT_EQ(l.callf<int>({"g", "f1"}, 1.5, 1.5), 3);
  EXPECT_EQ((l.callf<int, double, double>({"g", "f1"}, 1.5, 1.5)), 3);

  EXPECT_EQ(l.callf<int>({"g", "f2"}, 1, 1), 2);
  EXPECT_EQ((l.callf<std::tuple<int, int>>({"g", "f2"}, 1, 1)),
            (std::tuple<int, int>(2, 0)));
}
