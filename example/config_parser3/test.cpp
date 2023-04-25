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

#include <peacalm/lua_wrapper.h>

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
  peacalm::lua_wrapper l;

  int ret = l.dofile("./conf/knight.lua");
  if (ret != LUA_OK) {
    l.log_error_in_stack();
    return 0;
  }

  auto name     = l.get_string("name");
  auto country  = l.get_string("country");
  auto birthday = l.get_string("birthday", "unknown");
  auto power    = l.get_double("power");
  auto lifetime = l.get_long("lifetime");
  auto skills   = l.get<std::vector<std::string>>("skills");

  watch(name, country, birthday, power, lifetime, skills);
  return 0;
}
/* OUTPUT:
name = knight
country = Spain
birthday = 2000-1-1
power = 80
lifetime = 100
skills = [horse riding, archery, sword]
*/
