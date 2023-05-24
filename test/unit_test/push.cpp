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

TEST(push, push) {
  lua_wrapper l;

  // bool
  {
    l.settop(0);

    l.push(true);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_bool(), true);

    l.push(false);
    EXPECT_EQ(l.to<bool>(), false);
  }

  // integer
  {
    l.settop(0);

    l.push(3);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_int(), 3);

    l.push(-1);
    EXPECT_EQ(l.to_float(), -1);

    l.push(unsigned(0));
    EXPECT_EQ(l.to_int(), 0);

    l.push(short(5));
    EXPECT_EQ(l.to_int(), 5);

    l.push(unsigned(-1));
    EXPECT_EQ(l.to_int(), -1);
    EXPECT_EQ(l.to_uint(), unsigned(-1));

    l.push((1ULL << 63));
    EXPECT_EQ(l.to_int(), 0);
    EXPECT_EQ(l.to_llong(), 1LL << 63);
    EXPECT_EQ(l.to_ullong(), 1ULL << 63);
  }

  // float number
  {
    l.settop(0);

    l.push(3.5);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_float(), 3.5);
    EXPECT_EQ(l.to_double(), 3.5);
    EXPECT_EQ(l.to_ldouble(), 3.5);
    EXPECT_EQ(l.to<float>(), 3.5);
    EXPECT_EQ(l.to<double>(), 3.5);
    EXPECT_EQ(l.to<long double>(), 3.5);
  }

  // string
  {
    l.settop(0);

    std::string stdstr = "stdstr";
    l.push(stdstr);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_string(), stdstr);

    l.push("stdstr");
    EXPECT_EQ(l.to_string(), stdstr);

    const char *cstr = "cstr";
    l.push(cstr);
    EXPECT_EQ(l.to_string(), cstr);
  }

  // char array
  {
    l.settop(0);

    char        arr[4] = {'a', 'b', 'c', '\0'};
    const char *p      = arr;
    l.push(p);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_string(), "abc");
    l.push(p + 1);
    EXPECT_EQ(l.to_string(), "bc");

    // unsupported types for string:
    // l.push(arr);  // char[4], decay to char*
    // auto p1 = arr;  // char*
    // l.push(p1);
    // auto p2 = &arr;  // char(*)[4]
    // l.push(p2);
  }

  // const char array
  {
    l.settop(0);

    const char arr[4] = {'a', 'b', 'c', '\0'};
    l.push(arr);  // const char[4], decay to const char*
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_string(), "abc");

    const char *p = arr;
    l.push(p);
    EXPECT_EQ(l.to_string(), "abc");
    l.push(p + 1);
    EXPECT_EQ(l.to_string(), "bc");
    l.push(arr + 2);
    EXPECT_EQ(l.to_string(), "c");

    auto p1 = arr;  // const char*
    l.push(p1);
    EXPECT_EQ(l.to_string(), "abc");

    // unsupported types for string:
    // auto p2 = &arr;  // const char(*)[3]
    // l.push(p2);
  }

  // nullptr
  {
    l.settop(0);

    l.push(nullptr);
    EXPECT_EQ(l.gettop(), 1);

    EXPECT_TRUE(l.isnil());

    l.push(std::nullptr_t{});
    EXPECT_TRUE(l.isnil());

    const std::nullptr_t cnullptr{};
    l.push(cnullptr);
    EXPECT_TRUE(l.isnil());
  }

  // pair
  {
    l.settop(0);

    l.push(std::make_pair(1, 2.5));
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::pair<int, float>>()), (std::pair<int, float>(1, 2.5)));
    EXPECT_EQ((l.to<std::pair<bool, int>>()), (std::pair<bool, int>(true, 2)));

    l.push(std::make_pair(std::make_pair(1, "s"), 2.5));
    EXPECT_EQ((l.to<std::pair<std::pair<int, std::string>, float>>()),
              (std::pair<std::pair<int, std::string>, float>({1, "s"}, 2.5)));
  }

  // array
  {
    l.settop(0);

    l.push(std::array<int, 3>{1, 2, 3});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3}));
    // TODO
    // EXPECT_EQ((l.to<std::array<int, 3>>()), (std::array<int, 3>{1, 2, 3}));
  }

  // vector
  {
    l.settop(0);

    l.push(std::vector<int>{1, 2, 3, 4});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // deque
  {
    l.settop(0);

    l.push(std::deque<int>{1, 2, 3, 4});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // forward_list
  {
    l.settop(0);

    l.push(std::forward_list<int>{1, 2, 3, 4});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // list
  {
    l.settop(0);

    l.push(std::list<int>{1, 2, 3, 4});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // set
  {
    l.settop(0);

    l.push(std::set<int>{1, 2, 3});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::map<int, bool>>()),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));
  }

  // unordered_set
  {
    l.settop(0);

    l.push(std::unordered_set<int>{3, 1, 2});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::map<int, bool>>()),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));
  }

  // map
  {
    l.settop(0);

    l.push(std::map<int, bool>{{1, true}, {2, true}, {3, true}});
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::map<int, bool>>()),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));

    l.push(std::map<int, std::string>{{1, "one"}, {2, "two"}, {3, "three"}});
    EXPECT_EQ(
        (l.to<std::map<int, std::string>>()),
        (std::map<int, std::string>{{1, "one"}, {2, "two"}, {3, "three"}}));
  }

  // unordered_map
  {
    l.settop(0);
    l.push(std::unordered_map<int, std::string>{
        {1, "one"}, {2, "two"}, {3, "three"}});
    EXPECT_EQ(
        (l.to<std::map<int, std::string>>()),
        (std::map<int, std::string>{{1, "one"}, {2, "two"}, {3, "three"}}));
  }

  // complex
  {
    l.settop(0);
    l.push(std::make_pair(std::vector<int>{1, 2, 3},
                          std::unordered_map<int, std::string>{
                              {1, "one"}, {2, "two"}, {3, "three"}}));
    EXPECT_EQ((l.to<std::pair<std::vector<int>, std::map<int, std::string>>>()),
              (std::make_pair(std::vector<int>{1, 2, 3},
                              std::map<int, std::string>{
                                  {1, "one"}, {2, "two"}, {3, "three"}})));
  }
}

void func(int) {}

TEST(push, function) {
  lua_wrapper l;

  l.push(func);
  l.push(&func);
  l.push<lua_wrapper::function_tag>(func);
  l.push<lua_wrapper::function_tag>(&func);
  l.push<void(int)>(func);
  l.push<void(int)>(&func);

  // l.push<void (*)(int)>(func); // error with G++?
  // l.push<void (*)(int)>(&func);

  std::function<void(int)> f = func;
  l.push(f);
  l.push(std::move(f));
  l.push<lua_wrapper::function_tag>(f);
  l.push<lua_wrapper::function_tag>(std::move(f));

  auto l1 = [](int i) {};
  l.push(l1);
  l.push([](int i) {});
  l.push<void(int)>([](int i) {});
  l.push<lua_wrapper::function_tag>([](int i) {});

  auto l2 = [](auto x) { return x; };
  l.push<int(int)>(l2);
  l.push<double(double)>(l2);
  l.push<int(double)>(l2);
  l.push<double(int)>(l2);
}
