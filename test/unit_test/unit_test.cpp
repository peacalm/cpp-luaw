
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

#include "peacalm/lua_wrapper.h"

using namespace peacalm;

TEST(lua_wrapper, print_type_conversions_by_myostream) {
  lua_wrapper l;

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

TEST(lua_wrapper, type_conversion) {
  lua_wrapper l;

  // NONE
  std::cout << "Lua always convert NONE to 0" << std::endl;
  EXPECT_EQ(lua_toboolean(l.L(), -1), 0);
  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);
  EXPECT_EQ(lua_tonumber(l.L(), -1), 0);
  EXPECT_EQ(lua_tostring(l.L(), -1), nullptr);
  std::cout << "We convert NONE to default" << std::endl;
  EXPECT_EQ(l.to_bool(-1), false);
  EXPECT_EQ(l.to_bool(-1, true), true);
  EXPECT_EQ(l.to_int(-1), 0);
  EXPECT_EQ(l.to_llong(-1, -1), -1);
  EXPECT_EQ(l.to_double(-1, 1.5), 1.5);
  EXPECT_EQ(l.to_string(-1), "");

  // NIL
  l.settop(0);
  lua_pushnil(l.L());
  std::cout << "Lua always convert NIL to 0" << std::endl;
  EXPECT_EQ(lua_toboolean(l.L(), -1), 0);
  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);
  EXPECT_EQ(lua_tonumber(l.L(), -1), 0);
  EXPECT_EQ(lua_tostring(l.L(), -1), nullptr);
  std::cout << "We convert NIL to default" << std::endl;
  EXPECT_EQ(l.to_bool(-1), false);
  EXPECT_EQ(l.to_bool(-1, true), true);
  EXPECT_EQ(l.to_int(-1), 0);
  EXPECT_EQ(l.to_llong(-1, -1), -1);
  EXPECT_EQ(l.to_double(-1, 1.5), 1.5);
  EXPECT_EQ(l.to_string(-1), "");

  // Boolean
  l.settop(0);
  lua_pushboolean(l.L(), int(true));
  std::cout << "Lua doesn't convert bool to other types, return 0 on fail"
            << std::endl;
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);  // true to 0
  EXPECT_EQ(lua_tonumber(l.L(), -1), 0);   // true to 0
  EXPECT_EQ(lua_tostring(l.L(), -1), nullptr);
  std::cout << "We convert bool to number" << std::endl;
  EXPECT_EQ(l.to_bool(-1), true);
  EXPECT_EQ(l.to_int(-1), 1);          // convert true to 1
  EXPECT_EQ(l.to_llong(-1, -1), 1);    // convert true to 1
  EXPECT_EQ(l.to_double(-1, 2.5), 1);  // convert true to 1
  EXPECT_EQ(l.to_string(-1), "");      // can't convert to string

  // integer
  std::cout << "Lua convert any integer to bool true" << std::endl;
  l.settop(0);
  lua_pushinteger(l.L(), 0);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));  // 0 to true!
  l.settop(0);
  lua_pushinteger(l.L(), 1);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  l.settop(0);
  lua_pushinteger(l.L(), -1);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  l.settop(0);
  lua_pushinteger(l.L(), INT_MAX);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));

  l.settop(0);
  lua_pushinteger(l.L(), 3);
  std::cout << "Lua convert integer to bool, number and string" << std::endl;
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(lua_tointeger(l.L(), -1), 3);
  EXPECT_EQ(lua_tonumber(l.L(), -1), 3);
  EXPECT_EQ(strcmp(lua_tostring(l.L(), -1), "3"), 0);
  std::cout << "We convert integer to bool, number and string" << std::endl;
  EXPECT_EQ(l.to_bool(-1), true);  // integer to true
  EXPECT_EQ(l.to_int(-1), 3);
  EXPECT_EQ(l.to_llong(-1, -1), 3);
  EXPECT_EQ(l.to_long(-1, -1), 3);
  EXPECT_EQ(l.to_double(-1, 2.5), 3);
  EXPECT_EQ(l.to_string(-1, ""), std::string("3"));  // convert to string

  l.settop(0);
  lua_pushinteger(l.L(), 0);
  std::cout << "Lua convert integer 0 to boolean true!\n";
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(strcmp(lua_tostring(l.L(), -1), "0"), 0);
  std::cout << "We convert integer 0 to bool false" << std::endl;
  EXPECT_EQ(l.to_bool(-1), false);  // 0 to false
  EXPECT_EQ(l.to_int(-1), 0);
  EXPECT_EQ(l.to_string(-1, ""), std::string("0"));  // convert to string

  // string
  std::cout << "Lua convert any string to bool true" << std::endl;
  l.settop(0);
  lua_pushstring(l.L(), "");
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));  // '' to true
  l.settop(0);
  lua_pushstring(l.L(), "0");
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));  // '0' to true
  l.settop(0);
  lua_pushstring(l.L(), "1");
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  l.settop(0);
  lua_pushstring(l.L(), "abc");
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));

  std::cout << "Lua convert number like string to number" << std::endl;
  l.settop(0);
  lua_pushstring(l.L(), "-123");
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(lua_tointeger(l.L(), -1), -123);
  EXPECT_EQ(lua_tonumber(l.L(), -1), -123);
  EXPECT_EQ(strcmp(lua_tostring(l.L(), -1), "-123"), 0);

  std::cout << "Lua convert other string to 0" << std::endl;
  l.settop(0);
  lua_pushstring(l.L(), "other");
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);
  EXPECT_EQ(lua_tonumber(l.L(), -1), 0);

  std::cout << "We convert number like string to it's literal value. "
               "Especially '0' to false!"
            << std::endl;
  l.settop(0);
  lua_pushstring(l.L(), "-123");
  EXPECT_EQ(l.to_bool(-1), true);
  EXPECT_EQ(l.to_int(-1), -123);
  EXPECT_EQ(l.to_llong(-1, -1), -123);
  EXPECT_EQ(l.to_double(-1, 2.5), -123);
  EXPECT_EQ(l.to_string(-1), std::string("-123"));
  l.settop(0);
  lua_pushstring(l.L(), "0");
  EXPECT_EQ(l.to_bool(-1), false);

  std::cout << "We can't convert non-number-like-string to other types"
            << std::endl;
  l.settop(0);
  lua_pushstring(l.L(), "non-number-like-string");
  EXPECT_EQ(l.to_bool(-1), false);       // error, return default
  EXPECT_EQ(l.to_int(-1), 0);            // error, return default
  EXPECT_EQ(l.to_llong(-1, -1), -1);     // error, return default
  EXPECT_EQ(l.to_double(-1, 2.5), 2.5);  // error, return default
  EXPECT_EQ(l.to_string(-1), std::string("non-number-like-string"));

  EXPECT_EQ(l.gettop(), 1);

  // number
  std::cout << "Lua convert number to bool, string. 0 to true! 0 to '0.0'!"
            << std::endl;
  l.settop(0);
  lua_pushnumber(l.L(), 0);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));  // 0.0 to true
  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);
  EXPECT_EQ(lua_tonumber(l.L(), -1), 0);
  EXPECT_EQ(strcmp(lua_tostring(l.L(), -1), "0.0"), 0);
  std::cout << "We convert number to bool, string. 0 to false! 0 to '0.0'"
            << std::endl;
  EXPECT_EQ(l.to_bool(-1), false);  // integer 0 to false
  EXPECT_EQ(l.to_int(-1), 0);
  EXPECT_EQ(l.to_llong(-1, -1), 0);
  EXPECT_EQ(l.to_double(-1, 2.5), 0);
  EXPECT_EQ(l.to_string(-1), std::string("0.0"));  // convert to string

  l.settop(0);
  lua_pushnumber(l.L(), 1.0);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(l.to_string(-1), std::string("1.0"));  // convert to string

  EXPECT_EQ(lua_tointeger(l.L(), -1), 1);  // Lua: 1.0 to 1
  EXPECT_EQ(l.to_int(-1), 1);              // we: 1.0 to 1

  l.settop(0);
  lua_pushnumber(l.L(), 1.5);
  EXPECT_EQ(lua_toboolean(l.L(), -1), int(true));
  EXPECT_EQ(l.to_string(-1), std::string("1.5"));  // convert to string

  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);  // Lua: 1.5 to 0
  EXPECT_EQ(l.to_int(-1), 1);              // we: 1.5 to 1

  EXPECT_EQ(l.gettop(), 1);
}

TEST(lua_wrapper, long_number_like_string) {
  lua_wrapper l;
  const char *s = "123456789012345678901234567890";
  double      d = std::stod(s);
  lua_pushstring(l.L(), s);

  EXPECT_EQ(lua_tonumber(l.L(), -1), d);
  EXPECT_EQ(l.to_double(-1), d);
  // Lua can't convert integer if oversize, got 0
  EXPECT_EQ(lua_tointeger(l.L(), -1), 0);

  // We use C++ style static_cast, the result may be different by different
  // Compiler or Platform
  EXPECT_EQ(l.to_llong(-1), static_cast<long long>(d));

  EXPECT_EQ(l.gettop(), 1);

  l.reset();
  l.set_string("bignum", s);

  // The result may be different by different Compiler or Platform
  EXPECT_EQ(l.get_llong("bignum"),
            static_cast<long long>(d));  // value LLONG_MAX or LLONG_MIN
  EXPECT_EQ(l.get_ullong("bignum"),
            static_cast<unsigned long long>(d));  // value 0 or ULLONG_MAX

  EXPECT_EQ(l.get_double("bignum"), d);
  EXPECT_EQ(l.gettop(), 0);

  l.set_integer("bignum", LLONG_MAX);
  EXPECT_EQ(l.get_llong("bignum"), LLONG_MAX);
  EXPECT_EQ(l.get_ullong("bignum"), LLONG_MAX);
  EXPECT_EQ(l.get_double("bignum"), static_cast<double>(LLONG_MAX));

  l.set_integer("bignum", LLONG_MIN);
  EXPECT_EQ(l.get_llong("bignum"), LLONG_MIN);
  EXPECT_EQ(l.get_ullong("bignum"), 1ull << 63);

  // No ullong in Lua, this is equal to set -1 to 'bignum'
  l.set_integer("bignum", ULLONG_MAX);
  EXPECT_EQ(l.get_ullong("bignum"),
            ULLONG_MAX);  // we convert -1 to ull_max in C++
  EXPECT_EQ(l.get_llong("bignum"), -1);
  EXPECT_EQ(l.get_double("bignum"), -1);

  EXPECT_EQ(l.gettop(), 0);
}

TEST(lua_wrapper, large_number) {
  lua_wrapper l;
  l.reset();
  const char *s = "1921332203851725413";
  long long   i = std::stoll(s);
  assert(std::to_string(i) == s);
  double d = std::stod(s);
  l.set_string("s", s);
  lua_pushstring(l.L(), s);

  watch(s,
        lua_isinteger(l.L(), -1),
        lua_tointeger(l.L(), -1),
        l.get_llong("s"),
        std::stoll(s),
        std::stod(s),
        (long long)std::stod(s));

  EXPECT_EQ(l.get_llong("s"), i);
  EXPECT_EQ(l.get_llong("s"), d);
  EXPECT_EQ(l.get_double("s"), d);
  EXPECT_EQ(l.get_double("s"), i);
  EXPECT_NE(l.get_llong("s"), (long long)d);
  EXPECT_EQ((long long)l.get_double("s"), (long long)d);
  EXPECT_EQ(l.get_double("s"), double((long long)d));

  i = 6773679268829351174LL;
  l.set_integer("i", i);
  EXPECT_EQ(l.get_llong("i"), i);
  EXPECT_NE((long long)l.get_double("i"), i);
  EXPECT_EQ(l.get_double("i"), i);
}

TEST(lua_wrapper, type_conversion_failed_exists) {
  lua_wrapper l;
  {
    // none
    bool disable_log = false, failed = false, exists = false;
    EXPECT_EQ(l.to_bool(-1, true, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to_long(-1, -1, disable_log, &failed, &exists), -1);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to_double(-1, 1.5, disable_log, &failed, &exists), 1.5);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to_string(-1, "", disable_log, &failed, &exists), "");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);

    failed = false, exists = false;
    EXPECT_EQ(l.to<bool>(-1, disable_log, &failed, &exists), false);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to<int>(-1, disable_log, &failed, &exists), 0);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to<double>(-1, disable_log, &failed, &exists), 0.0);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to<std::string>(-1, disable_log, &failed, &exists),
              std::string{});
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }
  {
    // nil
    l.settop(0);
    lua_pushnil(l.L());
    bool disable_log = false, failed = false, exists = false;
    EXPECT_EQ(l.to_bool(-1, true, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to_long(-1, -1, disable_log, &failed, &exists), -1);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to_double(-1, 1.5, disable_log, &failed, &exists), 1.5);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to_string(-1, "", disable_log, &failed, &exists), "");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);

    failed = false, exists = false;
    EXPECT_EQ(l.to<bool>(-1, disable_log, &failed, &exists), false);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to<int>(-1, disable_log, &failed, &exists), 0);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to<double>(-1, disable_log, &failed, &exists), 0.0);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to<std::string>(-1, disable_log, &failed, &exists),
              std::string{});
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }
  {
    // bool
    l.settop(0);
    lua_pushboolean(l.L(), int(true));
    bool disable_log = false, failed = false, exists = false;
    EXPECT_EQ(l.to_bool(-1, false, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to_long(-1, -1, disable_log, &failed, &exists), 1);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to_double(-1, 1.5, disable_log, &failed, &exists), 1);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to_string(-1, "", disable_log, &failed, &exists), "");
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);

    failed = false, exists = false;
    EXPECT_EQ(l.to<bool>(-1, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to<int>(-1, disable_log, &failed, &exists), 1);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to<double>(-1, disable_log, &failed, &exists), 1);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to<std::string>(-1, disable_log, &failed, &exists),
              std::string{});
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
  }
  {
    // int
    l.settop(0);
    lua_pushinteger(l.L(), 123);
    bool disable_log = false, failed = false, exists = false;
    EXPECT_EQ(l.to_bool(-1, false, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to_long(-1, -1, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to_double(-1, 1.5, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to_string(-1, "", disable_log, &failed, &exists), "123");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    failed = false, exists = false;
    EXPECT_EQ(l.to<bool>(-1, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to<long long>(-1, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to<double>(-1, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to<std::string>(-1, disable_log, &failed, &exists), "123");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
  {
    // string
    l.settop(0);
    lua_pushstring(l.L(), "123.00");
    bool disable_log = false, failed = false, exists = false;
    EXPECT_EQ(l.to_bool(-1, false, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to_long(-1, -1, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to_double(-1, 1.5, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to_string(-1, "", disable_log, &failed, &exists), "123.00");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    failed = false, exists = false;
    EXPECT_EQ(l.to<bool>(-1, disable_log, &failed, &exists), true);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to<unsigned long long>(-1, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to<double>(-1, disable_log, &failed, &exists), 123);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to<std::string>(-1, disable_log, &failed, &exists), "123.00");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
  {
    // string
    l.settop(0);
    lua_pushstring(l.L(), "abc");
    bool disable_log = false, failed = false, exists = false;
    EXPECT_EQ(l.to_bool(-1, false, disable_log, &failed, &exists), false);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to_int(-1, -1, disable_log, &failed, &exists), -1);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to_double(-1, 1.5, disable_log, &failed, &exists), 1.5);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to_string(-1, "", disable_log, &failed, &exists), "abc");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    failed = false, exists = false;
    EXPECT_EQ(l.to<bool>(-1, disable_log, &failed, &exists), false);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = false;
    EXPECT_EQ(l.to<unsigned long>(-1, disable_log, &failed, &exists), 0);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    failed = false, exists = true;
    EXPECT_EQ(l.to<double>(-1, disable_log, &failed, &exists), 0);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    failed = true, exists = true;
    EXPECT_EQ(l.to<std::string>(-1, disable_log, &failed, &exists), "abc");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
}

TEST(lua_wrapper, set_and_get) {
  lua_wrapper l;

  l.set_boolean("b", true);
  l.set_integer("i", 5);
  l.set_number("f", 3.14);
  l.set_string("s", "Hello Lua!");

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_EQ(l.get_bool("b"), true);
  EXPECT_EQ(l.get_int("i"), 5);
  EXPECT_EQ(l.get_uint("i"), 5);
  EXPECT_EQ(l.get_long("i"), 5);
  EXPECT_EQ(l.get_ulong("i"), 5);
  EXPECT_EQ(l.get_double("f"), 3.14);
  EXPECT_EQ(l.get_string("s"), "Hello Lua!");
  EXPECT_EQ(strcmp(l.get_c_str("s"), "Hello Lua!"), 0);
  // get_c_str won't pop
  EXPECT_EQ(l.gettop(), 1);
  l.pop();  // we pop

  // max integer, min interger, -1
  l.set_integer("imax", LLONG_MAX);
  EXPECT_EQ(l.get_llong("imax"), LLONG_MAX);
  EXPECT_EQ(l.get_ullong("imax"), LLONG_MAX);
  EXPECT_EQ(l.get_int("imax"), -1);
  EXPECT_EQ(l.get_uint("imax"), UINT_MAX);
  l.set_integer("imin", LLONG_MIN);
  EXPECT_EQ(l.get_llong("imin"), LLONG_MIN);
  EXPECT_EQ(l.get_ullong("imin"),
            static_cast<unsigned long long>(LLONG_MAX) + 1ull);
  EXPECT_EQ(l.get_int("imin"), 0);
  EXPECT_EQ(l.get_uint("imin"), 0);
  l.set_integer("n1", -1);
  EXPECT_EQ(l.get_llong("n1"), -1);
  EXPECT_EQ(l.get_ullong("n1"), ULLONG_MAX);
  EXPECT_EQ(l.get_int("n1"), -1);
  EXPECT_EQ(l.get_uint("n1"), UINT_MAX);

  // clear
  l.set_nil("n1");
  EXPECT_EQ(l.get_int("n1"), 0);
  EXPECT_EQ(l.get_uint("n1"), 0);
  EXPECT_EQ(l.get_llong("n1"), 0);
  EXPECT_EQ(l.get_ullong("n1"), 0);
  {
    bool failed = false, exists = false;
    EXPECT_EQ(l.get_int("n1", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_uint("n1", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_llong("n1", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_ullong("n1", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_int("nxxx", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_uint("nxxx", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_llong("nxxx", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_ullong("nxxx", 0, true, &failed, &exists), 0);
    EXPECT_FALSE(exists);
  }

  // return default value
  EXPECT_EQ(l.get_int("n1", 1), 1);
  EXPECT_EQ(l.get_uint("n1", 2), 2);
  EXPECT_EQ(l.get_llong("n1", 3), 3);
  EXPECT_EQ(l.get_ullong("n1", 4), 4);

  EXPECT_EQ(l.get_string("nx", "def"), "def");
  EXPECT_EQ(l.get_string("nx"), "");

  EXPECT_EQ(l.gettop(), 0);

  // type conversion
  EXPECT_EQ(l.get_int("b"), 1);
  EXPECT_EQ(l.get_int("f"), 3);
  EXPECT_EQ(l.get_bool("i"), true);
  EXPECT_EQ(l.get_bool("f"), true);
  EXPECT_EQ(l.get_double("b"), 1);
  EXPECT_EQ(l.get_double("i"), 5);
  l.set_integer("i0", 0);
  l.set_string("s0", "0");
  EXPECT_EQ(l.get_bool("i0"), false);
  EXPECT_EQ(l.get_int("s0"), 0);
  EXPECT_EQ(l.get_bool("s0"), false);
  EXPECT_EQ(l.get_bool("none"), false);
  EXPECT_EQ(l.get_bool("none", true), true);
  l.set_boolean("bfalse", false);
  EXPECT_EQ(l.get_int("bfalse"), 0);
  EXPECT_EQ(l.get_int("bfalse", 1), 0);

  EXPECT_EQ(l.gettop(), 0);

  // number like string  <-> number
  l.set_string("si", "3.14");
  EXPECT_EQ(l.get_int("si"), 3);
  EXPECT_EQ(l.get_double("si"), 3.14);
  EXPECT_EQ(l.get_string("i"), "5");
  EXPECT_EQ(l.get_string("i0"), "0");
  EXPECT_EQ(l.get_string("f"), "3.14");

  EXPECT_EQ(l.gettop(), 0);

  // false conversion
  {
    bool failed = false;
    EXPECT_EQ(l.get_bool("s", false, true, &failed), false);
    EXPECT_TRUE(failed);
    failed = false;
    EXPECT_EQ(l.get_bool("s", true, true, &failed), true);
    EXPECT_TRUE(failed);
    failed = false;
    EXPECT_EQ(l.get_int("s", 0, true, &failed), 0);
    EXPECT_TRUE(failed);
    failed = false;
    EXPECT_EQ(l.get_int("s", -1, true, &failed), -1);
    EXPECT_TRUE(failed);
  }

  EXPECT_EQ(l.gettop(), 0);

  l.set_string("btrue", "true");
  EXPECT_EQ(l.get_bool("btrue"), false);
  EXPECT_EQ(l.get_int("btrue"), 0);

  EXPECT_EQ(l.get_string("b"), "");
  l.set_boolean("b", false);
  EXPECT_EQ(l.get_string("b"), "");

  EXPECT_EQ(l.gettop(), 0);
}

TEST(lua_wrapper, reset) {
  lua_wrapper l;

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

TEST(lua_wrapper, disable_log) {
  lua_wrapper l;

  l.set_string("x", "enable log once then disable!");
  bool disable_log = false, failed = false, exists = false;
  EXPECT_EQ(l.get_bool("x", false, disable_log, &failed, &exists), false);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  disable_log = true;
  failed      = false;
  exists      = false;
  EXPECT_EQ(l.get_bool("x", false, disable_log, &failed, &exists), false);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  failed = false;
  exists = false;
  EXPECT_EQ(l.get_int("x", 0, disable_log, &failed, &exists), 0);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  l.set_boolean("b", true);
  failed = false;
  exists = false;
  EXPECT_EQ(l.get_string("b", "", disable_log, &failed, &exists), "");
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  l.set_boolean("b", false);
  failed = false;
  exists = false;
  EXPECT_EQ(l.get_string("b", "", disable_log, &failed, &exists), "");
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ(l.gettop(), 0);
}

TEST(lua_wrapper, eval) {
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

TEST(lua_wrapper, eval_multi_ret) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_int("return 1,2,3"), 3);
}

struct vprovider {
  int def = 1;
  vprovider(int i = 1) : def(i) {
    // printf("vprovider(%d)\n", def);
  }
  ~vprovider() {
    // printf("~vprovider(%d)\n", def);
  }
  void provide(const std::string &v, lua_wrapper *l) { l->set_integer(v, def); }
  void provide(const std::vector<std::string> &vars, lua_wrapper *l) {
    for (const auto &v : vars) provide(v, l);
  }
};

std::set<std::string> toset(const std::vector<std::string> &v) {
  return std::set<std::string>(v.begin(), v.end());
}

TEST(lua_wrapper_crtp, detect_variable_names_eval) {
  lua_wrapper_is_provider<vprovider> l;

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

TEST(lua_wrapper_is_provider, auto_eval) {
  {
    lua_wrapper_is_provider<vprovider> l;
    EXPECT_EQ(l.auto_eval_int("return x"), 1);
  }
  {
    lua_wrapper_is_provider<vprovider> l(luaL_newstate());
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 3);
  }
  {
    lua_wrapper_is_provider<vprovider> l(luaL_newstate(), 2);
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 6);
  }
}

TEST(lua_wrapper_has_provider, auto_eval) {
  {
    lua_wrapper_has_provider<vprovider> l;
    EXPECT_EQ(l.auto_eval_int("return a"), 1);
  }
  {
    lua_wrapper_has_provider<vprovider *> l(luaL_newstate());
    l.provider(new vprovider);
    EXPECT_EQ(l.auto_eval_int("return a + b"), 2);
    delete l.provider();
  }
  {
    lua_wrapper_has_provider<std::shared_ptr<vprovider>> l{};
    l.provider(std::make_shared<vprovider>());
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 3);
  }
  {
    lua_wrapper_has_provider<std::unique_ptr<vprovider>> l;
    l.provider(std::make_unique<vprovider>(3));
    EXPECT_EQ(l.auto_eval_int("return a + b + c"), 9);
  }
}

TEST(lua_wrapper, IF) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_int("return IF(true, 1, 2)"), 1);
  EXPECT_EQ(l.eval_int("return IF(false, 1, 2)"), 2);
  EXPECT_EQ(l.eval_int("return IF(false, 1, '2')"), 2);
  EXPECT_EQ(l.eval_int("return IF(false, 1, '2.5')"), 2);
  EXPECT_EQ(l.eval_int("return IF(false, 1, 2.5)"), 2);
  EXPECT_EQ(l.eval_int("return IF(nil, 1, 2)"), 2);
  EXPECT_EQ(l.eval_int("return IF(1<0, 1, true, 3, 4)"), 3);
  EXPECT_EQ(l.eval_int("return IF(true and false, 1, nil, 3, 4)"), 4);
  EXPECT_EQ(l.eval_string("return IF(false, 1, nil, 3, 4)"), "4");
  EXPECT_EQ(l.eval_string("return IF(0, 'one', nil, 3, 4)"), "one");
  EXPECT_EQ(l.eval_string("return IF(not 0, 'one', 2^3 >= 8, 'three', 4)"),
            "three");
  EXPECT_EQ(l.eval_string("return IF(0>0, 'one', 2.5)"), "2.5");

  EXPECT_EQ(l.gettop(), 0);
}

TEST(lua_wrapper, SET) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[1]"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[3]"), false);
  EXPECT_EQ(l.eval_bool("return SET{1,2,4}[4]"), true);
  EXPECT_EQ(l.eval_bool("return SET{1.1,2.2}[2.2]"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[1] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[3] == nil"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[3] ~= true"), true);
  EXPECT_EQ(l.eval_bool("return SET{1,2,4}[4] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET{1.1 ,2.2}[2.2] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,'x','y',4)['x'] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,'x','y',4)['z'] == nil"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,nil,'x','y',4)['x'] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,nil,'x','y',4)['z'] == nil"), true);
  EXPECT_EQ(l.eval_bool("return SET{1,nil,'x','y',4}['x'] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET{1,nil,'x','y',4}['z'] == nil"), true);

  EXPECT_EQ(l.eval_bool("s={1,2,4} return SET(s)[4] == true"), true);
  EXPECT_EQ(l.eval_bool("s={1,nil,'x','y',4} return SET(s)['x'] == true"),
            true);

  EXPECT_EQ(l.eval_bool("s=SET{1,2,4} return s[4] == true"), true);
  EXPECT_EQ(l.eval_bool("s=SET(1,nil,'x','y',4) return s.x == true"), true);
}

TEST(lua_wrapper, COUNTER) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4)[1]"), 1);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,1,2,1)[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,1,2,1)[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,1,2,1)[2]"), 2);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,nil,1,2,1)[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,nil,1,2,1)[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,nil,1,2,1)[2]"), 2);

  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,1,2,1}[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,1,2,1}[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,1,2,1}[2]"), 2);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,nil,1,2,1}[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,nil,1,2,1}[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,nil,1,2,1}[2]"), 2);

  EXPECT_EQ(l.eval_int("c={1,2,4,1,2,1} return COUNTER(c)[2]"), 2);
  EXPECT_EQ(l.eval_int("c={1,2,4,nil,1,2,1} return COUNTER(c)[1]"), 3);

  EXPECT_EQ(l.eval_int("c=COUNTER{1,2,4,1,2,1} return c[2] + c[4]"), 3);
  EXPECT_EQ(l.eval_int("c=COUNTER(1,2,4,nil,1,2,1) return c[1]"), 3);

  EXPECT_TRUE(l.eval_bool("return COUNTER(1,2,3)[4] == nil"));
}

TEST(lua_wrapper, COUNTER0) {
  lua_wrapper l;
  EXPECT_TRUE(l.eval_bool("return COUNTER0(1,2,3)[4] == 0"));
  EXPECT_EQ(l.eval_int("return COUNTER0(1,2,3)[4]"), 0);

  EXPECT_EQ(l.eval_int("return COUNTER0(1,2,4,1,2,1)[1]"), 3);
  EXPECT_TRUE(l.eval_bool("return COUNTER0(1,2,4,1,2,1)[3] == 0"));

  EXPECT_EQ(l.gettop(), 0);
}

TEST(lua_wrapper, opt) {
  {
    lua_wrapper l;
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("return os.time()"), 0);
  }
  {
    lua_wrapper l(lua_wrapper::opt{}.preload_libs());
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("os = require 'os' ; return os.time()"), 0);
  }
  {
    lua_wrapper l(lua_wrapper::opt{}.ignore_libs());
    EXPECT_EQ(l.eval_int("--[[error]] return os.time()"), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    lua_wrapper l(lua_wrapper::opt{}.ignore_libs().register_exfunc(false));
    EXPECT_EQ(l.eval_int("--[[error]] return IF(true, 1, 2)"), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    lua_wrapper l(
        lua_wrapper::opt().custom_load(std::initializer_list<luaL_Reg>{
            {LUA_OSLIBNAME, luaopen_os},
            {NULL, NULL}}.begin()));
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("return os.time()"), 0);
  }
  {
    lua_wrapper l(
        lua_wrapper::opt().custom_preload(std::initializer_list<luaL_Reg>{
            {LUA_OSLIBNAME, luaopen_os},
            {NULL, NULL}}.begin()));
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GT(l.eval_int("os=require 'os'; return os.time()"), 0);
  }
  {
    lua_wrapper l(lua_wrapper::opt()
                      .custom_load(std::initializer_list<luaL_Reg>{
                          {LUA_GNAME, luaopen_base},
                          {LUA_OSLIBNAME, luaopen_os},
                          {NULL, NULL}}.begin())
                      .custom_preload(std::initializer_list<luaL_Reg>{
                          {LUA_MATHLIBNAME, luaopen_math},
                          {LUA_STRLIBNAME, luaopen_string},
                          {NULL, NULL}}.begin()));
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_GE(l.eval_int("m = require 'math' return m.sqrt(os.time())"), 40990);
  }
}

struct dummy_provider {
  int def = 0;
  dummy_provider(int i = 1) : def(i) {}
  bool provide(lua_State *L, const char *vname) {
    lua_pushnumber(L, def);
    return true;
  }
};
struct bad_provider {
  bool provide(lua_State *L, const char *vname) { return true; }
};
struct bad_provider2 {
  bool provide(lua_State *L, const char *vname) {
    lua_pushnumber(L, 0);
    lua_pushnumber(L, 1);
    return true;
  }
};
struct bad_provider3 {
  bool provide(lua_State *L, const char *vname) {
    lua_pushnumber(L, 0);
    return false;
  }
};

TEST(custom_lua_wrapper, eval) {
  custom_lua_wrapper<std::unique_ptr<dummy_provider>> l;
  l.provider(std::make_unique<dummy_provider>());
  EXPECT_EQ(l.eval_int("return a + b"), 2);
  l.provider(std::make_unique<dummy_provider>(2));
  EXPECT_EQ(l.eval_int("return x"), 2);
  EXPECT_EQ(l.eval_int("return a + b"), 4);
  EXPECT_EQ(l.eval_int("a = 4; return a + b"), 6);
}

TEST(custom_lua_wrapper, eval_failed) {
  {
    custom_lua_wrapper<std::unique_ptr<dummy_provider>> l;
    bool                                                failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_lua_wrapper<std::unique_ptr<dummy_provider>> l;
    l.provider(std::make_unique<dummy_provider>());
    lua_pushlightuserdata(l.L(), (void *)0);
    lua_setfield(l.L(), LUA_REGISTRYINDEX, "this");
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_lua_wrapper<std::unique_ptr<bad_provider>> l;
    l.provider(std::make_unique<bad_provider>());
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_lua_wrapper<std::unique_ptr<bad_provider2>> l;
    l.provider(std::make_unique<bad_provider2>());
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_lua_wrapper<std::unique_ptr<bad_provider3>> l;
    l.provider(std::make_unique<bad_provider3>());
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
}

TEST(lua_wrapper, template_type_conversion) {
  lua_wrapper l;
  lua_pushinteger(l.L(), 1);
  EXPECT_EQ(l.to<int>(), 1);
  EXPECT_EQ(l.to<long>(), 1);
  EXPECT_EQ(l.to<long long>(), 1);
  EXPECT_EQ(l.to<std::string>(), "1");
  // l.to<const char*>(); // building will fail

  {
    // failure conversions
    EXPECT_EQ((l.to<std::map<int, int>>()), (std::map<int, int>{}));
    lua_pushstring(l.L(), "abc");
    EXPECT_EQ((l.to<std::unordered_map<int, int>>()),
              (std::unordered_map<int, int>{}));
    lua_pushboolean(l.L(), true);
    EXPECT_EQ((l.to<std::vector<int>>()), (std::vector<int>{}));
  }
  l.settop(0);

  // vector
  {
    const char *expr = "t={1,2,3,4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>();
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1,2,'c'}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<std::string>>();
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<std::string>{"1", "2", "c"}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={a=1,b=2, 10,20}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>(-1);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{10, 20}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={{1,2},{3,4}}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<std::vector<int>>>(-1, true, nullptr);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<std::vector<int>>{{1, 2}, {3, 4}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, nil, nil, 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>(-1);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 4}));  // ignore nil
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>(-1);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    bool failed, exists;
    auto v = l.to<std::vector<std::string>>(-1, false, &failed, &exists);
    watch(expr, v, failed, exists);
    EXPECT_EQ(v, (std::vector<std::string>{"1", "2", "a", "b", "4"}));
    EXPECT_EQ(l.gettop(), sz);
  }

  // set
  {
    const char *expr = "t={1,2,3,4,3,2}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>();
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1,2,3,4,3}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::unordered_set<int>>();
    watch(expr, s);
    EXPECT_EQ(s, (std::unordered_set<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1,3,4,3}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::unordered_set<std::string>>();
    watch(expr, s);
    EXPECT_EQ(s, (std::unordered_set<std::string>{"1", "3", "4"}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={a=1,b=2, 10,20}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{10, 20}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, nil, 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{1, 2, 4}));  // ignore nil
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, nil, nil, 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::unordered_set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::unordered_set<int>{1, 2, 4}));  // ignore nil
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{1, 2, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    bool failed, exists;
    auto s = l.to<std::unordered_set<std::string>>(-1, false, &failed, &exists);
    watch(expr, s, failed, exists);
    EXPECT_EQ(s, (std::unordered_set<std::string>{"1", "2", "a", "b", "4"}));
    EXPECT_EQ(l.gettop(), sz);
  }

  // map
  {
    const char *expr = "m={a=1, b=2, c=3}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz       = l.gettop();
    using map_t  = std::map<std::string, long>;
    using umap_t = std::unordered_map<std::string, long>;
    auto m       = l.to<map_t>();
    auto um      = l.to<umap_t>();
    watch(expr, m, um);
    EXPECT_EQ(m, (map_t{{"a", 1}, {"b", 2}, {"c", 3}}));
    EXPECT_EQ(um, (umap_t{{"a", 1}, {"b", 2}, {"c", 3}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, b=nil, c=3}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz       = l.gettop();
    using map_t  = std::map<std::string, long>;
    using umap_t = std::unordered_map<std::string, long>;
    auto m       = l.to<map_t>();
    auto um      = l.to<umap_t>();
    watch(expr, m, um);
    EXPECT_EQ(m, (map_t{{"a", 1}, {"c", 3}}));
    EXPECT_EQ(um, (umap_t{{"a", 1}, {"c", 3}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, b=2, 3, 4}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz       = l.gettop();
    using map_t  = std::map<std::string, long>;
    using umap_t = std::unordered_map<std::string, long>;
    auto m       = l.to<map_t>(-1);
    auto um      = l.to<umap_t>(-1);
    watch(expr, m, um);
    EXPECT_EQ(m, (map_t{{"a", 1}, {"b", 2}, {"1", 3}, {"2", 4}}));
    EXPECT_EQ(um, (umap_t{{"a", 1}, {"b", 2}, {"1", 3}, {"2", 4}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, 1, 2, [3] = 3, [5]=5}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, int>>();
    auto um = l.to<std::unordered_map<int, int>>();
    watch(expr, m, um);
    EXPECT_EQ(m, (std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {5, 5}}));
    EXPECT_EQ(um,
              (std::unordered_map<int, int>{{1, 1}, {2, 2}, {3, 3}, {5, 5}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={[1]=1, [2]=2,  10, 20}";
    l.dostring(expr);
    l.getglobal("m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, int>>(-1, true, nullptr);
    auto um = l.to<std::unordered_map<int, int>>(-1, true, nullptr);
    watch(expr, m, um);
    EXPECT_EQ(m, (std::map<int, int>{{1, 10}, {2, 20}}));
    EXPECT_EQ(um, (std::unordered_map<int, int>{{1, 10}, {2, 20}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={10, 20, [1]=1, [2]=2}";
    l.dostring(expr);
    l.getglobal("m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, int>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m, (std::map<int, int>{{1, 10}, {2, 20}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, b=2, 11, true, F=false}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, int>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, int>{
                  {"a", 1}, {"b", 2}, {"1", 11}, {"2", 1}, {"F", 0}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1.0, b=2.5, c=3, 1.1, 2.2, true, F=false}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, double>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, double>{{"a", 1},
                                             {"b", 2.5},
                                             {"c", 3},
                                             {"1", 1.1},
                                             {"2", 2.2},
                                             {"3", 1},
                                             {"F", 0}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1.0, b=2.5, c=3, 1.1, 2.2, true, F=false}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::unordered_map<std::string, double>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::unordered_map<std::string, double>{{"a", 1},
                                                       {"b", 2.5},
                                                       {"c", 3},
                                                       {"1", 1.1},
                                                       {"2", 2.2},
                                                       {"3", 1},
                                                       {"F", 0}}));
    EXPECT_EQ(l.gettop(), sz);
  }

  // complex
  {
    const char *expr = "m={a={1,3}, b={2,4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, std::vector<int>>>(-1);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, std::vector<int>>{{"a", {1, 3}},
                                                       {"b", {2, 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={[10010]={1,3}, [10086]={2,4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, std::vector<int>>>(-1);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, std::vector<int>>{{"10010", {1, 3}},
                                                       {"10086", {2, 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={[10010]={1,3}, [10086]={2,4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, std::vector<int>>>(-1);
    watch(expr, m);
    EXPECT_EQ(
        m, (std::map<int, std::vector<int>>{{10010, {1, 3}}, {10086, {2, 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a={X=1,Y=3}, b={X=2,Y=4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz     = l.gettop();
    using type = std::unordered_map<std::string, std::map<std::string, int>>;
    auto m     = l.to<type>(-1);
    watch(expr, m);
    EXPECT_EQ(m,
              (type{{"a", {{"X", 1}, {"Y", 3}}}, {"b", {{"X", 2}, {"Y", 4}}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
}

TEST(lua_wrapper, template_get) {
  lua_wrapper l;
  bool        failed, exists;
  l.dostring(
      "a = 1; b = {1,2}; c={'x', 'y', 'z'}; d={a=1,b=2}; e={ {a=1,b=2}, "
      "{a=2,b=1} }");
  EXPECT_EQ(l.get<long>("a"), 1);
  EXPECT_EQ(l.get<long>("a", false, &failed, &exists), 1);
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ(l.get<std::vector<int>>("b"), (std::vector<int>{1, 2}));
  EXPECT_EQ(l.get<std::vector<int>>("b", false, &failed, &exists),
            (std::vector<int>{1, 2}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ(l.get<std::vector<std::string>>("c"),
            (std::vector<std::string>{"x", "y", "z"}));
  EXPECT_EQ(l.get<std::vector<std::string>>("c", false, &failed, &exists),
            (std::vector<std::string>{"x", "y", "z"}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::unordered_map<std::string, int>>(
                "d", false, &failed, &exists)),
            (std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  l.set_nil("d");
  EXPECT_EQ((l.get<std::unordered_map<std::string, int>>(
                "d", false, &failed, &exists)),
            (std::unordered_map<std::string, int>{}));
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);

  using type = std::vector<std::map<std::string, int>>;
  EXPECT_EQ(l.get<type>("e"),
            (type{{{"a", 1}, {"b", 2}}, {{"a", 2}, {"b", 1}}}));
  EXPECT_EQ(l.get<type>("e", false, &failed, &exists),
            (type{{{"a", 1}, {"b", 2}}, {{"a", 2}, {"b", 1}}}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  l.dostring(
      "a = '1' a2 = false a3 = true a4 ='x'; "
      "b = {1, '2', 'x'} b2 = {1, '2', '3.5'}; "
      "c = {'x', 'y', 'z', false}; "
      "d = {a=1,b=2, [33]=3} d2 = {1,2,['3']=3, ['4']=4}; "
      "d3 = {'a', 2, c='c'}");
  EXPECT_EQ(l.get<long>("a"), 1);
  EXPECT_EQ(l.get<long>("a", false, &failed, &exists), 1);
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);
  EXPECT_EQ(l.get<long>("a2", false, &failed, &exists), 0);
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);
  EXPECT_EQ(l.get<long>("a3", false, &failed, &exists), 1);
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);
  EXPECT_EQ(l.get<long>("a4", false, &failed, &exists), 0);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  EXPECT_EQ(l.get<long>("axxx", false, &failed, &exists), 0);
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);

  EXPECT_EQ(l.get<std::vector<int>>("b", false, &failed, &exists),
            (std::vector<int>{1, 2}));
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  EXPECT_EQ(l.get<std::vector<int>>("b2", false, &failed, &exists),
            (std::vector<int>{1, 2, 3}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ(l.get<std::vector<std::string>>("c"),
            (std::vector<std::string>{"x", "y", "z"}));
  EXPECT_EQ(l.get<std::vector<std::string>>("c", false, &failed, &exists),
            (std::vector<std::string>{"x", "y", "z"}));
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ(
      (l.get<std::unordered_map<std::string, int>>(
          "d", false, &failed, &exists)),
      (std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}, {"33", 3}}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::unordered_map<std::string, int>>(
                "d2", false, &failed, &exists)),
            (std::unordered_map<std::string, int>{
                {"1", 1}, {"2", 2}, {"3", 3}, {"4", 4}}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::map<int, int>>("d2", false, &failed, &exists)),
            (std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {4, 4}}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::unordered_map<std::string, int>>(
                "d3", false, &failed, &exists)),
            (std::unordered_map<std::string, int>{{"2", 2}}));
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::map<int, int>>("d3", false, &failed, &exists)),
            (std::map<int, int>{{2, 2}}));
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::unordered_map<std::string, std::string>>(
                "d3", false, &failed, &exists)),
            (std::unordered_map<std::string, std::string>{
                {"1", "a"}, {"2", "2"}, {"c", "c"}}));
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  l.dostring("d=nil d2=nil d3=nil");
  EXPECT_EQ((l.get<std::map<int, int>>("d2", false, &failed, &exists)),
            (std::map<int, int>{}));
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);
  EXPECT_EQ((l.get<std::map<std::string, int>>("d3", false, &failed, &exists)),
            (std::map<std::string, int>{}));
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);

  l.dostring("d4={a=1, b=true, c=false, d=nil, e='s'}");
  EXPECT_EQ((l.get<std::map<std::string, int>>("d4", false, &failed, &exists)),
            (std::map<std::string, int>{{"a", 1}, {"b", 1}, {"c", 0}}));
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  EXPECT_EQ((l.get<std::map<std::string, std::string>>(
                "d4", false, &failed, &exists)),
            (std::map<std::string, std::string>{{"a", "1"}, {"e", "s"}}));
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
}

TEST(lua_wrapper, template_eval) {
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
}

TEST(lua_wrapper, seek) {
  lua_wrapper l;
  l.gseek("g");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.isnil(-1));
  l.seek("gg");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.isnil(-1));
  l.seek(1);
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_TRUE(l.isnil(-1));

  l.settop(0);
  l.dostring("g={a=1, gg={a=11,b='bb'}, list={1,2,3}}");
  l.gseek("g");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.istable(-1));
  l.seek("a");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_EQ(l.to<int>(), 1);
  l.pop();
  l.seek("gg");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.istable(-1));
  l.seek("a");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_EQ(l.to<int>(), 11);
  l.pop();
  l.seek("b");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_EQ(l.to<std::string>(), "bb");
  l.pop(2);
  l.seek("list");
  int list_idx = l.gettop();
  EXPECT_EQ(l.seek(2).to_int(), 2);
  l.pop();
  EXPECT_EQ(l.seek(3).to<long>(), 3);
  EXPECT_EQ(l.seek(1, list_idx).to_double(), 1);
  EXPECT_EQ(l.gettop(), 4);

  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek("gg").seek("a").to_int(), 11);
  EXPECT_EQ(l.gseek("g").seek("list").seek(2).to_int(), 2);
  EXPECT_EQ(l.gettop(), 6);

  l.seek(3);
  EXPECT_TRUE(l.isnil());
  l.pop(2);
  l.seek(3);
  EXPECT_FALSE(l.isnil());
  EXPECT_TRUE(l.isinteger());
  EXPECT_TRUE(l.isnumber());
  EXPECT_EQ(l.to_uint(), 3);

  l.settop(0);
  l.dostring("g={{1,2,3.0}, {'a', 'b', 'c'}, m={{a=1},{a=2}} }");
  EXPECT_EQ(l.gseek("g").seek(1).seek(1).to_int(), 1);
  EXPECT_EQ(l.gettop(), 3);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).seek(3).to_int(), 3);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).seek(3).to_string(), "3.0");
  EXPECT_EQ(l.gettop(), 3);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(2).seek(3).to<std::string>(), "c");
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek("m").seek(1).seek("a").to_int(), 1);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek("m").seek(2).seek("a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 4);

  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).to<std::vector<int>>(),
            (std::vector<int>{1, 2, 3}));
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).to<std::vector<std::string>>(),
            (std::vector<std::string>{"1", "2", "3.0"}));
  EXPECT_EQ(l.gettop(), 2);

  l.pop();
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_EQ((l.seek("m").seek(2).to<std::map<std::string, int>>()),
            (std::map<std::string, int>{{"a", 2}}));
  EXPECT_EQ(l.gettop(), 3);

  l.seek(3);
  EXPECT_TRUE(l.isnil());
  EXPECT_EQ(l.gettop(), 4);
  l.pop();

  l.settop(0);
}

TEST(lua_wrapper, recursive_get) {
  lua_wrapper l;
  l.dostring("a={b={c=3, d=2.0},b2=2, b3={1,2,1}} b=true s='s' d=2.5");

  EXPECT_EQ(l.get<int>({"a", "b", "c"}), 3);
  EXPECT_EQ(l.get<int>({std::string("a"), std::string("b"), std::string("c")}),
            3);
  EXPECT_EQ(l.get<int>(std::vector<const char *>{"a", "b", "c"}), 3);
  EXPECT_EQ(l.get<int>(std::vector<std::string>{"a", "b", "c"}), 3);
  EXPECT_EQ(l.get<int>(std::initializer_list<const char *>{"a", "b", "c"}), 3);
  EXPECT_EQ(l.get<int>(std::initializer_list<std::string>{"a", "b", "c"}), 3);
  EXPECT_EQ(l.get<int>(
                std::initializer_list<std::string>{std::string("a"), "b", "c"}),
            3);
  EXPECT_EQ(l.gettop(), 0);

  EXPECT_EQ(l.get<int>({"a", "b2"}), 2);
  EXPECT_EQ(l.get<std::set<int>>({"a", "b3"}), (std::set<int>{1, 2}));
  EXPECT_EQ(l.get<std::vector<int>>({"a", "b3"}), (std::vector<int>{1, 2, 1}));

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_EQ(l.get<bool>({"b"}), true);
  EXPECT_EQ(l.get<std::string>({"s"}), "s");
  EXPECT_EQ(l.get<double>({"d"}), 2.5);

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_EQ(l.get<bool>(std::vector<std::string>{}), false);
  EXPECT_EQ(l.get<std::string>(std::initializer_list<const char *>{}), "");
  EXPECT_EQ(l.get<std::string>(std::initializer_list<std::string>{}), "");

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_EQ(l.get_int({"a", "b", "c"}), 3);
  EXPECT_EQ(l.get_string({"a", "b", "c"}), "3");
  EXPECT_EQ(l.get_double({"a", "b", "d"}), 2);
  EXPECT_EQ(l.get_string({"a", "b", "d"}), "2.0");
  EXPECT_EQ(l.get_ullong({"a", "b2"}), 2);
  EXPECT_EQ(l.get_bool({"b"}), true);

  EXPECT_EQ(l.gettop(), 0);

  {
    bool failed, exists;
    EXPECT_EQ(l.get_string({"a", "b", "c"}, "def", false, &failed, &exists),
              "3");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(l.get_string({"a", "b", "x"}, "def", false, &failed, &exists),
              "def");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_string({"a", "bx"}, "def", false, &failed, &exists), "def");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get_string({"aa", "b", "d"}, "def", false, &failed, &exists),
              "def");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);

    EXPECT_EQ(l.get_string({"a", "b"}, "def", false, &failed, &exists), "def");
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(l.get_string({"a"}, "def", false, &failed, &exists), "def");
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(l.get_string(
                  std::vector<std::string>{}, "def", false, &failed, &exists),
              "def");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }
  {
    bool failed, exists;
    EXPECT_EQ(l.get<std::string>({"a", "b", "c"}, false, &failed, &exists),
              "3");
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(l.get<std::string>({"a", "b", "x"}, false, &failed, &exists), "");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get<std::string>({"a", "bx"}, false, &failed, &exists), "");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
    EXPECT_EQ(l.get<std::string>({"aa", "b", "d"}, false, &failed, &exists),
              "");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);

    EXPECT_EQ(l.get<std::string>({"a", "b"}, false, &failed, &exists), "");
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(l.get<std::string>({"a"}, false, &failed, &exists), "");
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(
        l.get<std::string>(std::vector<std::string>{}, false, &failed, &exists),
        "");
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }

  {
    bool failed, exists;
    l.dostring("g={a={1,2,nil,4}, b={1,true,3}, c={1.0,'x'} }");

    auto ga = l.get<std::vector<int>>({"g", "a"}, false, &failed, &exists);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(ga, (std::vector<int>{1, 2, 4}));

    auto gb = l.get<std::vector<int>>({"g", "b"}, false, &failed, &exists);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(gb, (std::vector<int>{1, 1, 3}));

    auto gbs =
        l.get<std::vector<std::string>>({"g", "b"}, false, &failed, &exists);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(gbs, (std::vector<std::string>{"1", "3"}));

    auto gcs =
        l.get<std::vector<std::string>>({"g", "c"}, false, &failed, &exists);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(gcs, (std::vector<std::string>{"1.0", "x"}));

    auto gcf = l.get<std::vector<double>>({"g", "c"}, false, &failed, &exists);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
    EXPECT_EQ(gcf, (std::vector<double>{1}));
  }
}

TEST(lua_wrapper, eval_using_setted_global) {
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

TEST(lua_wrapper, abs_index) {
  lua_wrapper l;
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
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  std::cout << ">>> Running lua_wrapper unit test." << std::endl;

  int ret = RUN_ALL_TESTS();

  return ret;
}