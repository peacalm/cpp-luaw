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

TEST(constructions, opt) {
  {
    luaw l;
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("return os.time()"), 0);
  }
  {
    luaw l(luaw::opt{}.preload_libs());
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("os = require 'os' ; return os.time()"), 0);
  }
  {
    luaw l(luaw::opt{}.ignore_libs());
    EXPECT_EQ(l.eval_int("--[[error]] return os.time()"), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    luaw l(luaw::opt{}.ignore_libs().register_exfunctions(false));
    EXPECT_EQ(l.eval_int("--[[error]] return IF(true, 1, 2)"), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    luaw l(
        luaw::opt().ignore_libs().custom_load({{LUA_OSLIBNAME, luaopen_os}}));
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("return os.time()"), 0);
  }
  {
    luaw l(luaw::opt().ignore_libs().custom_preload(
        {{LUA_OSLIBNAME, luaopen_os}}));
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("os=require 'os'; return os.time()"), 0);
  }
  {
    luaw l(luaw::opt()
               .ignore_libs()
               .custom_load({{LUA_GNAME, luaopen_base},
                             {LUA_LOADLIBNAME, luaopen_package},
                             {LUA_OSLIBNAME, luaopen_os}})
               .custom_preload({{LUA_MATHLIBNAME, luaopen_math},
                                {LUA_STRLIBNAME, luaopen_string}}));
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GE(l.eval_int("m = require 'math' return m.sqrt(os.time())"), 40990);
  }
}

TEST(constructions, reset) {
  luaw l;

  l.set_boolean("b", true);
  l.set_integer("i", 5);
  l.set_number("f", 3.14);
  l.set_string("s", "Hello Lua!");

  l.reset();
  EXPECT_EQ(l.get_bool("b"), false);
  EXPECT_EQ(l.get_int("i"), 0);
  EXPECT_EQ(l.get_double("f"), 0);
  EXPECT_EQ(l.get_string("s"), "");
}

TEST(constructions, release_and_move_ctor_and_move_assign) {
  // release
  {
    luaw       l;
    lua_State *L = l.release();
    EXPECT_TRUE(L);
    EXPECT_FALSE(l.L());
    lua_close(L);
  }
  // move ctor
  {
    luaw a;
    auto al = a.L();
    luaw b(std::move(a));
    EXPECT_FALSE(a.L());
    EXPECT_EQ(al, b.L());
    b.dostring("print('b is moved from a!')");
  }
  {
    luaw a(nullptr);
    auto al = a.L();
    luaw b(std::move(a));
    EXPECT_FALSE(a.L());
    EXPECT_FALSE(b.L());
  }
  // move assign
  {
    luaw a, b;
    auto al = a.L();
    auto bl = b.L();
    // normal assignment
    b = std::move(a);
    EXPECT_FALSE(a.L());
    EXPECT_TRUE(b.L());
    EXPECT_EQ(al, b.L());
    b.dostring("print('b is move assigned from a!')");
  }
  {
    luaw a;
    auto al = a.L();
    // self assignment
    a = std::move(a);
    EXPECT_TRUE(a.L());
    EXPECT_EQ(al, a.L());
    a.dostring("print('self assignment: a is not changed!')");
  }
  {
    auto L = luaL_newstate();
    luaL_openlibs(L);
    // wrapper of same L
    luaw a(L), b(L);
    b = std::move(a);
    EXPECT_FALSE(a.L());
    EXPECT_TRUE(b.L());
    EXPECT_EQ(L, b.L());
    b.dostring("print('wrapper of same L: b is not changed!')");
  }
  {
    luaw a(nullptr), b;
    b = std::move(a);
    EXPECT_FALSE(a.L());
    EXPECT_FALSE(b.L());
  }
  {
    luaw a, b(nullptr);
    auto al = a.L();
    b       = std::move(a);
    EXPECT_FALSE(a.L());
    EXPECT_TRUE(b.L());
    EXPECT_EQ(al, b.L());
    b.dostring("print('b is move assigned from a!')");
  }
}
