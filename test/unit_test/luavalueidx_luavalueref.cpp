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

TEST(luavalueidx, luavalueidx) {
  luaw l;
  l.push(1);
  l.push(nullptr);
  {
    auto lv = l.to<luaw::luavalueidx>(1);
    EXPECT_TRUE(lua_isinteger(lv.L(), lv.idx()));
  }
  {
    auto lv = l.to<luaw::luavalueidx>(2);
    EXPECT_TRUE(lua_isnil(lv.L(), lv.idx()));
  }
  {
    auto lv = l.to<luaw::luavalueidx>(3);
    EXPECT_TRUE(lua_isnone(lv.L(), lv.idx()));
  }

  EXPECT_EQ(l.gettop(), 2);
}

TEST(luavalueref, luavalueref) {
  luaw l;
  l.push(1);
  l.push(nullptr);
  {
    auto lr = l.to<luaw::luavalueref>(1);
    watch("ref of 1", lr.ref_id());
    lr.getvalue();
    EXPECT_TRUE(l.isinteger());
    EXPECT_EQ(l.to_int(), 1);
    l.pop();
  }
  EXPECT_EQ(l.gettop(), 2);
  {
    auto lr = l.to<luaw::luavalueref>(2);
    watch("ref of nil", lr.ref_id());
    lr.getvalue();
    EXPECT_TRUE(l.isnil());
    l.pop();
  }
  EXPECT_EQ(l.gettop(), 2);
  {
    // ref on none got nil
    auto lr = l.to<luaw::luavalueref>(3);
    watch("ref of none", lr.ref_id());
    lr.getvalue();
    // ref on none got nil
    EXPECT_FALSE(lua_isnone(lr.L(), -1));
    EXPECT_TRUE(lua_isnil(lr.L(), -1));
    l.pop();
  }
  EXPECT_EQ(l.gettop(), 2);
}

TEST(luavalueidx, as_func_arg) {
  luaw l;
  l.dostring("g = {a=8,b=8}");

  auto getter = [](const luaw::luavalueidx& t, const std::string& k) {
    luaw_fake l(t.L());
    l.gseek("g").seek(k);
    return luaw::luavalueref(t.L());
  };
  auto setter = [](const luaw::luavalueidx& t,
                   const std::string&       k,
                   const luaw::luavalueidx& v) {
    luaw_fake l(t.L());
    l.gseek("g").setkv(k, v);
  };

  l.dostring("t = {}");

  l.lset<luaw::function_tag>("t", luaw::metatable_tag{}, "__index", getter);
  l.lset<luaw::function_tag>("t", luaw::metatable_tag{}, "__newindex", setter);

  l.eval<void>("t.a = 1; t.c = 's';");

  EXPECT_EQ(l.eval<int>("return t.a"), 1);
  EXPECT_EQ(l.eval<int>("return g.a"), 1);

  EXPECT_EQ(l.eval<int>("return t.b"), 8);
  EXPECT_EQ(l.eval<int>("return g.b"), 8);

  EXPECT_EQ(l.eval<std::string>("return t.c"), "s");
  EXPECT_EQ(l.eval<std::string>("return g.c"), "s");

  EXPECT_EQ(l.eval<int>("return t.d"), 0);
  EXPECT_EQ(l.eval<int>("return g.d"), 0);

  EXPECT_EQ(l.get<long>({"t", "a"}), 1);
  EXPECT_EQ(l.get<long>({"g", "b"}), 8);
  EXPECT_EQ(l.get<std::string>({"g", "c"}), "s");
  EXPECT_EQ(l.lget<long>({}, "g", "b"), 8);
  EXPECT_EQ(l.lget<std::string>({}, "g", "c"), "s");
}
