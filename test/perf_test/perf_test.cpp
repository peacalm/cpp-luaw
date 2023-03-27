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

#include <gtest/gtest.h>

#include <iostream>

#if defined(ENABLE_MYOSTREAM_WATCH)
#include <myostream.h>
#define watch(...)                        \
  std::cout << MYOSTREAM_WATCH_TO_STRING( \
      std::string, " = ", "\n", "\n\n", __VA_ARGS__)
#define watch_with_std_cout(...) \
  MYOSTREAM_WATCH(std::cout, " = ", "\n", "\n\n", __VA_ARGS__)
#else
#define watch(...)
#define watch_with_std_cout(...)
#endif

#include "peacalm/lua_wrapper.h"

using namespace peacalm;

struct provider {
  bool cache = false;
  provider(bool c = false) : cache(c) {}

  bool provide(lua_State* L, const char* vname) {
    char s[2]  = {0, 0};
    bool found = false;
    for (int i = 0; i < 26; ++i) {
      s[0] = 'a' + i;
      if (strcmp(vname, s) == 0) {
        lua_pushnumber(L, i + 1);
        found = true;
        break;
      }
    }
    if (!found) return false;
    if (cache) {
      lua_pushvalue(L, -1);
      lua_setglobal(L, vname);
    }
    return true;
  }

  void provide(const std::vector<std::string>& vars, lua_wrapper* l) {
    for (const auto& v : vars) {
      for (int i = 0; i < 26; ++i) {
        if (v == std::string{char('a' + i)}) {
          l->set_number(v, i + 1);
          break;
        }
      }
    }
  }
};

const char* expr =
    "return a + b - c * d + e / f * g ^ h - x * p - q * n / s + v - m + c ^ k";
const int rep = 100000;

TEST(custom_lua_wrapper, eval_no_cache) {
  custom_lua_wrapper<std::unique_ptr<provider>> l;
  l.provider(std::make_unique<provider>(false));
  double ret;
  for (int i = 0; i < rep; ++i) { ret = l.eval_double(expr); }
  watch(ret);
}

TEST(custom_lua_wrapper, eval_cache) {
  custom_lua_wrapper<std::unique_ptr<provider>> l;
  l.provider(std::make_unique<provider>(true));
  double ret;
  for (int i = 0; i < rep; ++i) { ret = l.eval_double(expr); }
  watch(ret);
}

TEST(lua_wrapper_has_provider, eval_cache) {
  lua_wrapper_has_provider<std::unique_ptr<provider>> l;
  l.provider(std::make_unique<provider>(true));
  double ret;
  for (int i = 0; i < rep; ++i) { ret = l.auto_eval_double(expr); }
  watch(ret);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  std::cout << ">>> Running lua_wrapper perf test." << std::endl;

  int ret = RUN_ALL_TESTS();

  return ret;
}