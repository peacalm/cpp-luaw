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

#include <gtest/gtest.h>

#include <initializer_list>
#include <iostream>
#include <string>

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

#include "peacalm/luaw.h"

using namespace peacalm;

struct provider {
  bool cache = false;
  provider(bool c = false) : cache(c) {}

  bool provide(luaw& l, const char* vname) {
    char s[2]  = {0, 0};
    bool found = false;
    for (int i = 0; i < 26; ++i) {
      s[0] = 'a' + i;
      if (strcmp(vname, s) == 0) {
        l.push(i + 1);
        found = true;
        break;
      }
    }
    if (!found) return false;
    if (cache) l.copy_to_global(vname);
    return true;
  }

  void provide(const std::vector<std::string>& vars, luaw& l) {
    for (const auto& v : vars) {
      for (int i = 0; i < 26; ++i) {
        if (v == std::string{char('a' + i)}) {
          l.set_number(v, i + 1);
          break;
        }
      }
    }
  }
};

const char* expr =
    "return a + b - c * d + e / f * g ^ h - x * p - q * n / s + v - m + c ^ k";
const int rep = 10000;

TEST(custom_luaw, re_init_eval) {
  double ret;
  for (int i = 0; i < rep; ++i) {
    custom_luaw<std::unique_ptr<provider>> l;
    l.provider(std::make_unique<provider>(false));
    ret = l.eval_double(expr);
  }
  watch(ret);
}

TEST(custom_luaw, re_init_no_exfunc_eval) {
  double ret;
  for (int i = 0; i < rep; ++i) {
    custom_luaw<std::unique_ptr<provider>> l(
        luaw::opt{}.register_exfunctions(false));
    l.provider(std::make_unique<provider>(false));
    ret = l.eval_double(expr);
  }
  watch(ret);
}

TEST(custom_luaw, re_init_nolib_eval) {
  double ret;
  for (int i = 0; i < rep; ++i) {
    custom_luaw<std::unique_ptr<provider>> l(
        luaw::opt{}.ignore_libs().register_exfunctions(false));
    l.provider(std::make_unique<provider>(false));
    for (int i = 0; i < 26; ++i) {
      l.set_number(std::string{char('a' + i)}, i + 1);
    }
    ret = l.eval_double(expr);
  }
  watch(ret);
}

TEST(custom_luaw, re_init_preload_eval) {
  double ret;
  for (int i = 0; i < rep; ++i) {
    custom_luaw<std::unique_ptr<provider>> l(luaw::opt{}.preload_libs());
    l.provider(std::make_unique<provider>(false));
    ret = l.eval_double(expr);
  }
  watch(ret);
}

TEST(custom_luaw, re_init_custom_load_eval) {
  double ret;
  for (int i = 0; i < rep; ++i) {
    custom_luaw<std::unique_ptr<provider>> l(
        luaw::opt{}.ignore_libs().custom_load({{LUA_GNAME, luaopen_base}}));
    l.provider(std::make_unique<provider>(false));
    ret = l.eval_double(expr);
  }
  watch(ret);
}

TEST(custom_luaw, re_init_raw_provider_ptr_eval) {
  double    ret;
  provider* p = new provider;
  for (int i = 0; i < rep; ++i) {
    custom_luaw<provider*> l;
    l.provider(p);
    ret = l.eval_double(expr);
  }
  watch(ret);
  delete p;
}

TEST(custom_luaw, eval_no_cache) {
  custom_luaw<std::unique_ptr<provider>> l;
  l.provider(std::make_unique<provider>(false));
  double ret;
  for (int i = 0; i < rep; ++i) { ret = l.eval_double(expr); }
  watch(ret);
}

TEST(custom_luaw, eval_cache) {
  custom_luaw<std::unique_ptr<provider>> l;
  l.provider(std::make_unique<provider>(true));
  double ret;
  for (int i = 0; i < rep; ++i) { ret = l.eval_double(expr); }
  watch(ret);
}

TEST(luaw_has_provider, eval_cache) {
  luaw_has_provider<std::unique_ptr<provider>> l;
  l.provider(std::make_unique<provider>(true));
  double ret;
  for (int i = 0; i < rep; ++i) { ret = l.auto_eval_double(expr); }
  watch(ret);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  std::cout << ">>> Running Luaw perf test." << std::endl;

  int ret = RUN_ALL_TESTS();

  return ret;
}
