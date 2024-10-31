// Copyright (c) 2023-2024 Li Shuangquan. All Rights Reserved.
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

#include <peacalm/luaw.h>

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

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

int main() {
  peacalm::luaw l;

  int ret = l.dofile("conf.lua");
  if (ret != LUA_OK) {
    l.log_error_in_stack();
    return 1;
  }

  int         a = l.get_int("a");
  double      b = l.get_double("b");
  long        c = l.get_llong("c");
  std::string d = l.get_string("d");

  auto v        = l.get<std::vector<int>>("v");
  auto mii_by_v = l.get<std::map<int, int>>("v");

  auto m   = l.get<std::map<std::string, int>>("m");
  auto m_c = l.get_string({"m", "c"}, "nx");

  auto g_flag      = l.get<std::map<std::string, std::string>>({"g", "flag"});
  auto g_bigcities = l.get<std::vector<std::string>>({"g", "bigcities"});
  auto g_flag_star_num = l.get<int>({"g", "flag", "star_num"});
  auto g_population    = l.get_llong({"g", "population"}, -1);

  watch(a, b, c, d, v, mii_by_v, m, m_c, g_flag, g_bigcities, g_population);
}
// clang-format off
/* OUTPUT:
a = 1
b = 3.14159
c = 1000000000123
d = good
v = [1, 3, 5, 7]
mii_by_v = {1: 1, 2: 3, 3: 5, 4: 7}
m = {a: 1, b: 2, c: 3, d: 4}
m_c = 3
g_flag = {bgcolor: red, star_color: yellow, star_num: 5, star_orientation: upper left}
g_bigcities = [Beijing, Shanghai, Guangzhou, Shenzhen]
g_population = -1
*/
