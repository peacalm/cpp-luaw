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

#include "main.h"

#if 0

TEST(watch_type_conversion, watch_lua_rule_and_my_rule) {
  luaw l;

#define full_watch(info)          \
  watch(info,                     \
        lua_isinteger(l.L(), -1), \
        lua_isnumber(l.L(), -1),  \
        lua_isstring(l.L(), -1),  \
        lua_isboolean(l.L(), -1), \
        l.to_llong(-1),           \
        l.to_ullong(-1),          \
        l.to_double(-1),          \
        l.to_string(-1),          \
        l.to_bool(-1),            \
        lua_tointeger(l.L(), -1), \
        lua_tonumber(l.L(), -1),  \
        lua_tostring(l.L(), -1),  \
        lua_toboolean(l.L(), -1))

  lua_pushinteger(l.L(), 0);
  full_watch("pushinteger 0");

  lua_pushinteger(l.L(), 2);
  full_watch("pushinteger 2");

  lua_pushinteger(l.L(), LLONG_MAX);
  full_watch("pushinteger LLONG_MAX");

  lua_pushinteger(l.L(), 7213265539493896576);
  full_watch("pushinteger 7213265539493896576");

  lua_pushnumber(l.L(), 2.5);
  full_watch("pushnumber 2.5");

  lua_pushnumber(l.L(), 2.0);
  full_watch("pushnumber 2.0");

  lua_pushnumber(l.L(), 0.0);
  full_watch("pushnumber 0.0");

  lua_pushstring(l.L(), "2.5");
  full_watch("pushstring '2.5'");

  lua_pushstring(l.L(), "0");
  full_watch("pushstring '0'");

  lua_pushstring(l.L(), "7213265539493896576");
  full_watch("pushstring '7213265539493896576'");
  watch((long long)(double)(7213265539493896576));

  lua_pushstring(l.L(), "+7213265539493896576");
  full_watch("pushstring '+7213265539493896576'");

  lua_pushstring(l.L(), "-7213265539493896576");
  full_watch("pushstring '-7213265539493896576'");

  lua_pushstring(l.L(), "12345678901234567890");  // > 2^63-1, <2^64-1
  full_watch("pushstring '12345678901234567890'");

  lua_pushstring(l.L(), "123456789012345678901234567890");  // > 2^64-1
  full_watch("pushstring '123456789012345678901234567890'");

  lua_pushstring(l.L(), "abc");
  full_watch("pushstring 'abc'");

  lua_pushboolean(l.L(), true);
  watch("pushboolean true",
        lua_isinteger(l.L(), -1),
        lua_isnumber(l.L(), -1),
        lua_isstring(l.L(), -1),
        lua_isboolean(l.L(), -1),
        l.to_llong(-1),
        l.to_double(-1),
        l.to_string(-1),
        l.to_bool(-1),
        lua_tointeger(l.L(), -1),
        lua_tonumber(l.L(), -1),
        // lua_tostring(l.L(), -1), // panic
        lua_toboolean(l.L(), -1));
}

#endif
