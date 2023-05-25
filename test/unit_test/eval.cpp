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

TEST(eval, eval) {
  lua_wrapper l;

  // Error! Lua returns '', C++ returns default and prints an error log
  EXPECT_EQ(l.eval_bool("return ''"), false);
  // OK! Lua converts '' to true
  EXPECT_EQ(l.eval_bool("return not not ''"), true);

  EXPECT_EQ(l.eval_bool("return 0"), false);
  EXPECT_EQ(l.eval_bool("return 1"), true);
  EXPECT_EQ(l.eval_bool("return -1"), true);
  EXPECT_EQ(l.eval_bool("return 123"), true);

  EXPECT_EQ(l.eval_int("return 2^3"), 8);
  EXPECT_EQ(l.eval_int("return 2^3 - 9"), -1);

  EXPECT_EQ(l.eval_double("return 3/2"), 1.5);
  EXPECT_EQ(l.eval_double("return 3//2"), 1);

  EXPECT_EQ(l.eval_string("return 'Hello'"), "Hello");
  EXPECT_EQ(l.eval_string("if 0 then return 'A' else return 'B' end"), "A");
  EXPECT_EQ(l.eval_string("if false then return 'A' else return 'B' end"), "B");

  EXPECT_EQ(l.gettop(), 0);

  l.set_integer("a", 1);
  l.set_integer("b", 2);
  l.set_integer("c", 3);
  l.set_integer("d", 4);
  EXPECT_EQ(l.eval_int("return a + b + c + d"), 10);
  l.dostring("e = a + b + c + d");
  EXPECT_EQ(l.get_int("e"), 10);
  EXPECT_EQ(l.eval_int("return e"), 10);
  EXPECT_EQ(l.eval_ulong("return e"), 10);

  EXPECT_EQ(l.eval_double("return a + b * c / d"), 1 + 2 * 3 / 4.0);

  EXPECT_EQ(l.gettop(), 0);

  l.eval_string("s = 'a' .. '0' ");
  l.eval_int("return 1,2,3");
  l.get_string("s");

  watch(l.eval_string("s = 'a' .. '0' "),
        l.eval_int("return 1,2,3"),
        l.get_string("s"));

  EXPECT_EQ(l.gettop(), 0);
}

TEST(eval, eval_multi_ret) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_int("return 1,2,3"), 1);
  EXPECT_EQ(l.eval<long>("return 1,2,3"), 1);
  EXPECT_EQ(l.eval<std::string>("return 1,2,3"), "1");
}

TEST(eval, template_eval) {
  lua_wrapper l;
  {
    const char *expr = "return {1,2,3}";
    auto        v    = l.eval<std::vector<unsigned>>(expr);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<unsigned>{1, 2, 3}));
  }
  {
    const char *expr = "return {x=1,y=2}";
    auto        v    = l.eval<std::unordered_map<std::string, int>>(expr);
    watch(expr, v);
    EXPECT_EQ(v, (std::unordered_map<std::string, int>{{"x", 1}, {"y", 2}}));
  }
  {
    const char *expr = "return {[2.2]=2,[2.6]=3, x=1, y=2, 'bug'}";
    auto        v    = l.eval<std::unordered_map<double, int>>(expr);
    watch(expr, v);
    EXPECT_EQ(v, (std::unordered_map<double, int>{{2.2, 2}, {2.6, 3}}));
  }
  {
    const char *expr = "return {false, true}";
    auto        v    = l.eval<std::pair<bool, bool>>(expr);
    watch(expr, v);
    EXPECT_EQ(v, (std::pair<bool, bool>{false, true}));
  }
}

TEST(eval, eval_using_setted_global) {
  lua_wrapper l;
  l.set_integer("a", 10);
  l.set_integer("b", 5);
  l.set_integer("c", 2);

  double ret = l.eval_double("return a^2 + b/c");  // 102.5

  std::string s = l.eval_string(
      "if a > b + c then return 'good' else return 'bad' end");  // "good"

  auto si = l.eval<std::set<int>>("return {a, b, c}");  // {2,5,10}

  EXPECT_EQ(ret, 102.5);
  EXPECT_EQ(s, "good");
  EXPECT_EQ(si, (std::set<int>{2, 5, 10}));
}

TEST(eval, tuple) {
  lua_wrapper l;
  {
    bool failed;
    auto t = l.eval<std::tuple<bool, int, std::string>>(
        "return true, 2, 'tuple' ", false, &failed);
    EXPECT_EQ(t, std::make_tuple(true, 2, "tuple"));
    EXPECT_FALSE(failed);
  }
  {
    bool failed;
    auto t = l.eval<std::tuple<>>("a=true b=2 c='tuple' ", false, &failed);
    EXPECT_EQ(t, std::make_tuple());
    EXPECT_FALSE(failed);
  }
  {
    bool failed;
    auto t =
        l.eval<const std::tuple<>>("a=true b=2 c='tuple' ", false, &failed);
    EXPECT_EQ(t, std::make_tuple());
    EXPECT_FALSE(failed);
  }

  // l.eval<void>("a=true b=2 c='tuple' "); // error
}
