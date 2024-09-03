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

/// @deprecated Use custom_luaw instead.

#include "main.h"

namespace {
struct vprovider {
  int def = 1;
  vprovider(int i = 1) : def(i) {
    // printf("vprovider(%d)\n", def);
  }
  ~vprovider() {
    // printf("~vprovider(%d)\n", def);
  }
  void provide(const std::string &v, luaw &l) { l.set_integer(v, def); }
  void provide(const std::vector<std::string> &vars, luaw &l) {
    for (const auto &v : vars) provide(v, l);
  }
};

std::set<std::string> toset(const std::vector<std::string> &v) {
  return std::set<std::string>(v.begin(), v.end());
}

}  // namespace

TEST(luaw_crtp, detect_variable_names_eval) {
  luaw_is_provider<vprovider> l;

  EXPECT_EQ(toset(l.detect_variable_names("return a + b")), toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a + 50")), toset({"a"}));
  EXPECT_EQ(toset(l.detect_variable_names("return 20 + 50")), toset({}));
  EXPECT_EQ(toset(l.detect_variable_names("return _a * b_")),
            toset({"_a", "b_"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "if x then return _a * b_ else return y end")),
            toset({"_a", "b_", "x", "y"}));
  EXPECT_EQ(toset(l.detect_variable_names("a = 1; b = 2; return a + b")),
            toset({}));
  EXPECT_EQ(
      toset(l.detect_variable_names("a = 'str'; b = '2'; return a .. b .. c")),
      toset({"c"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "a = [[str 'str' \"haha\"]]; b = '2'; return a .. b .. c")),
            toset({"c"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "[[str 'str' \"haha\"]]; b = '2'; return a .. b .. c")),
            toset({"a", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a..b .. c")),
            toset({"a", "b", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a .. b .. c")),
            toset({"a", "b", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a .. \"b\" .. c")),
            toset({"a", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a .. \"b\" .. c .. 12")),
            toset({"a", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a .. \"bb'sbb's\" .. c")),
            toset({"a", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a .. \"bb's b b's\" .. c")),
            toset({"a", "c"}));
  EXPECT_EQ(
      toset(l.detect_variable_names("return a .. \"bb's b b's \\\"d e\" .. c")),
      toset({"a", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "return a .. \"bb's b b's \\\"d - e\" .. c")),
            toset({"a", "c"}));
  EXPECT_EQ(
      toset(l.detect_variable_names("return a .. 'bb\\'s b  \\\"d - e' .. c")),
      toset({"a", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "return a .. [['bb's b  \\\"d - e']] .. c")),
            toset({"a", "c"}));

  //
  EXPECT_EQ(toset(l.detect_variable_names(
                "--[[ a + b ; 'aa' .. 2 --]] return a + b")),
            toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "--[=[ a + b ; 'aa' .. 2 c * d]] --]=] return a + b")),
            toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a--[[name of var]] + b")),
            toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names(
                "--[--[ x + y ; 'aa' .. 2 c * d ]]\n return a + b")),
            toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a --[-[name of var]]\n + b")),
            toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a--[-[name of var]]\n + b")),
            toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a-b")), toset({"a", "b"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a- -b")), toset({"a", "b"}));

  //
  EXPECT_EQ(toset(l.detect_variable_names("return a + (b * c)")),
            toset({"a", "b", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a + f(b * c)")),
            toset({"a", "b", "c"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a + math.pi")), toset({"a"}));
  EXPECT_EQ(toset(l.detect_variable_names("return a + b.c.d")), toset({"a"}));
}

TEST(luaw_is_provider, auto_eval) {
  {
    luaw_is_provider<vprovider> l;
    EXPECT_EQ(l.auto_eval_int("return x"), 1);
  }
  {
    luaw_is_provider<vprovider> l(luaL_newstate());
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 3);
  }
  {
    luaw_is_provider<vprovider> l(luaL_newstate(), 2);
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 6);
  }
}

TEST(luaw_has_provider, auto_eval) {
  {
    luaw_has_provider<vprovider> l;
    EXPECT_EQ(l.auto_eval_int("return a"), 1);
  }
  {
    luaw_has_provider<vprovider *> l(luaL_newstate());
    l.provider(new vprovider);
    EXPECT_EQ(l.auto_eval_int("return a + b"), 2);
    delete l.provider();
  }
  {
    luaw_has_provider<std::shared_ptr<vprovider>> l{};
    l.provider(std::make_shared<vprovider>());
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 3);
  }
  {
    luaw_has_provider<std::unique_ptr<vprovider>> l;
    l.provider(std::make_unique<vprovider>(3));
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 9);
  }
}
