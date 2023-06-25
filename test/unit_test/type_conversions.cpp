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

TEST(type_conversions, simple_types) {
  luaw l;

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
  std::cout << "We convert number to bool, string. 0 to false! 0 to '0.0'!"
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

TEST(type_conversions, long_number_like_string) {
  luaw        l;
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

TEST(type_conversions, large_number) {
  luaw l;
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

TEST(type_conversions, test_failed_exists) {
  luaw l;
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

TEST(type_conversions, template_to_complex_types) {
  luaw l;
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

  // pair
  {
    EXPECT_EQ((l.to<std::pair<int, int>>()), (std::pair<int, int>{}));
    const char *expr = "t={1,2,3,4,x=1,y=2}";
    l.dostring(expr);
    l.getglobal("t");
    EXPECT_EQ((l.to<std::pair<int, int>>()), (std::pair<int, int>{1, 2}));
    EXPECT_EQ(l.gettop(), 1);

    l.dostring("t[1] = 2 t[2] = 3.5");
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::pair<int, int>>()), (std::pair<int, int>{2, 3}));
    EXPECT_EQ((l.to<std::pair<int, double>>()),
              (std::pair<int, double>{2, 3.5}));
    EXPECT_EQ((l.to<std::pair<bool, double>>()),
              (std::pair<bool, double>{true, 3.5}));
    EXPECT_EQ((l.to<std::pair<std::string, double>>()),
              (std::pair<std::string, double>{"2", 3.5}));

    l.dostring("t[1] = nil t[2] = 3.5");
    EXPECT_EQ((l.to<std::pair<std::string, double>>()),
              (std::pair<std::string, double>{"", 3.5}));
    l.dostring("t[1] = nil t[2] = nil");
    EXPECT_EQ((l.to<std::pair<std::string, double>>()),
              (std::pair<std::string, double>{"", 0}));
    EXPECT_EQ((l.to<std::pair<bool, double>>()), (std::pair<bool, double>{}));

    l.dostring("t[1] = true t[2] = {1,2}");
    EXPECT_EQ((l.to<std::pair<bool, std::pair<int, int>>>()),
              (std::pair<bool, std::pair<int, int>>{true, {1, 2}}));

    l.dostring("t[1] = true t[2] = 2.5 t = {} ");
    EXPECT_EQ((l.to<std::pair<bool, double>>()),
              (std::pair<bool, double>{true, 2.5}));
    EXPECT_EQ((l.to<std::pair<long, double>>()),
              (std::pair<long, double>{1, 2.5}));
    EXPECT_EQ(l.gettop(), 1);

    l.getglobal("t");
    EXPECT_EQ((l.to<std::pair<bool, double>>()), (std::pair<bool, double>{}));
    EXPECT_EQ((l.to<std::pair<long, double>>()), (std::pair<long, double>{}));
    EXPECT_EQ(l.gettop(), 2);

    l.settop(0);
  }

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

  // deque, list, forward_list
  {
    const char *expr = "t={1,2,3,4}";
    l.dostring(expr);
    l.getglobal("t");
    int sz = l.gettop();
    EXPECT_EQ(l.to<std::deque<int>>(), (std::deque<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.to<std::list<int>>(), (std::list<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.to<std::forward_list<int>>(),
              (std::forward_list<int>{1, 2, 3, 4}));

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
    int  sz   = l.gettop();
    auto m    = l.to<std::map<std::string, std::vector<int>>>(-1);
    auto mpii = l.to<std::map<std::string, std::pair<int, int>>>(-1);
    watch(expr, m, mpii);
    EXPECT_EQ(m,
              (std::map<std::string, std::vector<int>>{{"a", {1, 3}},
                                                       {"b", {2, 4}}}));
    EXPECT_EQ(mpii,
              (std::map<std::string, std::pair<int, int>>{{"a", {1, 3}},
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
  {
    const char *expr = "p={{X=1,Y=3}, {X=2,Y=4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "p");
    int sz     = l.gettop();
    using type = std::pair<std::unordered_map<std::string, int>,
                           std::map<std::string, int>>;
    auto m     = l.to<type>(-1);
    watch(expr, m);
    EXPECT_EQ(m, (type{{{"X", 1}, {"Y", 3}}, {{"X", 2}, {"Y", 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
}

TEST(type_conversions, to_tuple) {
  luaw l;
  l.dostring("t = {true, 1, 2.5, 'str'}");
  l.gseek("t");
  EXPECT_EQ(l.gettop(), 1);

  {
    bool failed, exists;
    auto t = l.to<std::tuple<bool, int, double, std::string>>(
        -1, false, &failed, &exists);
    EXPECT_EQ(t, std::make_tuple(true, 1, 2.5, "str"));
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
  {
    bool failed, exists;
    auto t = l.to<std::tuple<bool, int>>(-1, false, &failed, &exists);
    EXPECT_EQ(t, std::make_tuple(true, 1));
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
  {
    bool failed, exists;
    auto t = l.to<std::tuple<bool, int>>(2, false, &failed, &exists);
    EXPECT_EQ(t, std::make_tuple(false, 0));
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }

  {
    bool failed, exists;
    auto t =
        l.to<std::tuple<bool, std::tuple<int, double>, double, std::string>>(
            1, false, &failed, &exists);
    watch(t);
    EXPECT_EQ(t, std::make_tuple(true, std::make_tuple(0, 0.0), 2.5, "str"));
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
  }

  {
    // empty tuple
    bool failed, exists;
    auto t = l.to<std::tuple<>>(1, false, &failed, &exists);
    EXPECT_EQ(t, std::make_tuple());
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
  {
    // empty tuple
    bool failed, exists;
    auto t = l.to<std::tuple<>>(2, false, &failed, &exists);
    EXPECT_EQ(t, std::make_tuple());
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }
  {
    l.dostring("t = {true, {1, 2, 3}, 'str'}");
    l.gseek("t");

    EXPECT_EQ((l.to<std::tuple<bool, std::pair<int, int>, std::string>>()),
              std::make_tuple(true, std::make_pair(1, 2), std::string("str")));

    EXPECT_EQ(
        (l.to<std::tuple<bool, std::vector<int>, std::string>>()),
        std::make_tuple(true, std::vector<int>{1, 2, 3}, std::string("str")));

    EXPECT_EQ(
        (l.to<std::tuple<bool, std::tuple<int, int, int>, std::string>>()),
        std::make_tuple(true, std::make_tuple(1, 2, 3), std::string("str")));
  }
}

TEST(type_conversions, to_void) {
  luaw l;
  l.push(1);
  l.push(nullptr);

  bool failed, exists;
  l.to<void>(1, false, &failed, &exists);
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  l.to<void>(2, false, &failed, &exists);
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);
}

TEST(type_conversions, to_function) {
  luaw l;
  l.dostring("f = function(a, b) return a + b end");
  l.getglobal("f");
  EXPECT_EQ(l.gettop(), 1);
  l.push(nullptr);
  EXPECT_EQ(l.gettop(), 2);
  int sz = l.gettop();

  {
    bool failed, exists;
    auto f = l.to<luaw::function<int(int, int)>>(1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), 3);
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    // get a const luaw::function

    bool       failed, exists;
    const auto f =
        l.to<const luaw::function<int(int, int)>>(1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), 3);
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    bool failed, exists;
    auto f = l.to<luaw::function<int(int, int)>>(2, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_TRUE(failed);
    EXPECT_FALSE(exists);

    EXPECT_EQ(f(1, 2), 0);
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_TRUE(f.function_failed());
    EXPECT_FALSE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_FALSE(f.result_exists());
    EXPECT_TRUE(f.failed());
  }

  auto f2 = l.to<luaw::function<void(int, int)>>(1);
  f2(2, 3);

  auto f3 = l.to<std::function<int(int, int)>>(1);
  EXPECT_EQ(f3(1, 2), 3);

  auto f4 = l.to<std::function<void(int, int)>>(1);
  f4(2, 3);

  EXPECT_EQ(l.gettop(), sz);
  {
    auto f = l.to<luaw::function<double(double &, double &)>>(1);
    EXPECT_EQ(l.gettop(), sz);

    double a = 2.25, b = 1.25;
    EXPECT_EQ(f(a, b), 3.5);
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_EQ(a, 2.25);
    EXPECT_EQ(b, 1.25);
  }
}

TEST(type_conversions, to_function_with_tuple_result) {
  luaw l;
  l.dostring("f = function(a, b) return a + b, a - b, a * b end");
  l.getglobal("f");
  EXPECT_EQ(l.gettop(), 1);
  int sz = l.gettop();

  {
    bool failed, exists;
    auto f = l.to<luaw::function<std::tuple<int, int, int>(int, int)>>(
        1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(3, -1, 2));

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    // const tuple as result
    bool failed, exists;
    auto f = l.to<luaw::function<const std::tuple<int, int, int>(int, int)>>(
        1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(3, -1, 2));

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    // result size 3 -> 4
    bool failed, exists;
    auto f = l.to<luaw::function<std::tuple<int, int, int, int>(int, int)>>(
        1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(3, -1, 2, 0));

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    // result size 3 -> 2
    bool failed, exists;
    auto f = l.to<luaw::function<std::tuple<int, int>(int, int)>>(
        1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(3, -1));

    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    // result size 3 -> 1
    bool failed, exists;
    auto f = l.to<luaw::function<int(int, int)>>(1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), 3);
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
}

TEST(type_conversions, to_function_with_nested_tuple_result) {
  luaw l;
  l.dostring("f = function(a, b) return true, {a + b, a - b, a * b} end");
  l.getglobal("f");
  EXPECT_EQ(l.gettop(), 1);
  int sz = l.gettop();

  {
    bool failed, exists;
    auto f = l.to<
        luaw::function<std::tuple<bool, std::tuple<int, int, int>>(int, int)>>(
        1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(true, std::make_tuple(3, -1, 2)));
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    bool failed, exists;
    auto f =
        l.to<luaw::function<std::tuple<bool, std::tuple<int, int>>(int, int)>>(
            1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(true, std::make_tuple(3, -1)));
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
  {
    bool failed, exists;
    auto f = l.to<luaw::function<std::tuple<bool, std::tuple<int>>(int, int)>>(
        1, false, &failed, &exists);

    EXPECT_EQ(l.gettop(), sz);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);

    EXPECT_EQ(f(1, 2), std::make_tuple(true, std::make_tuple(3)));
    EXPECT_EQ(l.gettop(), sz);

    EXPECT_FALSE(f.function_failed());
    EXPECT_TRUE(f.function_exists());
    EXPECT_FALSE(f.result_failed());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.failed());
  }
}

TEST(type_conversions, to_pointer) {
  luaw       l;
  static int x = 0;
  l.pushlightuserdata(&x);
  l.push(nullptr);
  l.push(123);

  bool failed, exists;

  // void*

  EXPECT_EQ(l.to<void *>(1, false, &failed, &exists), (void *)(&x));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, true);

  EXPECT_EQ(l.to<const void *>(1, false, &failed, &exists), (const void *)(&x));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, true);

  EXPECT_EQ(l.to<volatile void *>(1, false, &failed, &exists),
            (volatile void *)(&x));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, true);

  // nil

  EXPECT_FALSE(l.to<void *>(2, false, &failed, &exists));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, false);
  EXPECT_FALSE(l.to<const void *>(2, false, &failed, &exists));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, false);
  EXPECT_FALSE(l.to<volatile void *>(2, false, &failed, &exists));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, false);

  // integer

  EXPECT_FALSE(l.to<void *>(3, false, &failed, &exists));
  EXPECT_EQ(failed, true);
  EXPECT_EQ(exists, true);
  EXPECT_FALSE(l.to<const void *>(3, false, &failed, &exists));
  EXPECT_EQ(failed, true);
  EXPECT_EQ(exists, true);
  EXPECT_FALSE(l.to<volatile void *>(3, false, &failed, &exists));
  EXPECT_EQ(failed, true);
  EXPECT_EQ(exists, true);

  // none

  EXPECT_FALSE(l.to<void *>(8, false, &failed, &exists));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, false);
  EXPECT_FALSE(l.to<const void *>(8, false, &failed, &exists));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, false);
  EXPECT_FALSE(l.to<volatile void *>(8, false, &failed, &exists));
  EXPECT_EQ(failed, false);
  EXPECT_EQ(exists, false);
}
