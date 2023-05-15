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
