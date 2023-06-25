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
  luaw l;

  // bool
  {
    l.settop(0);

    EXPECT_EQ(l.push(true), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_bool(), true);

    EXPECT_EQ(l.push(false), 1);
    EXPECT_EQ(l.to<bool>(), false);
  }

  // integer
  {
    l.settop(0);

    EXPECT_EQ(l.push(3), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_int(), 3);

    EXPECT_EQ(l.push(-1), 1);
    EXPECT_EQ(l.to_float(), -1);

    EXPECT_EQ(l.push(unsigned(0)), 1);
    EXPECT_EQ(l.to_int(), 0);

    EXPECT_EQ(l.push(short(5)), 1);
    EXPECT_EQ(l.to_int(), 5);

    EXPECT_EQ(l.push(unsigned(-1)), 1);
    EXPECT_EQ(l.to_int(), -1);
    EXPECT_EQ(l.to_uint(), unsigned(-1));

    EXPECT_EQ(l.push((1ULL << 63)), 1);
    EXPECT_EQ(l.to_int(), 0);
    EXPECT_EQ(l.to_llong(), 1LL << 63);
    EXPECT_EQ(l.to_ullong(), 1ULL << 63);
  }

  // float number
  {
    l.settop(0);

    EXPECT_EQ(l.push(3.5), 1);
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
    EXPECT_EQ(l.push(stdstr), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_string(), stdstr);

    EXPECT_EQ(l.push("stdstr"), 1);
    EXPECT_EQ(l.to_string(), stdstr);

    const char *cstr = "cstr";
    EXPECT_EQ(l.push(cstr), 1);
    EXPECT_EQ(l.to_string(), cstr);
  }

  // char array
  {
    l.settop(0);

    char        arr[4] = {'a', 'b', 'c', '\0'};
    const char *p      = arr;
    EXPECT_EQ(l.push(p), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_string(), "abc");
    EXPECT_EQ(l.push(p + 1), 1);
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
    EXPECT_EQ(l.push(arr), 1);  // const char[4], decay to const char*
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to_string(), "abc");

    const char *p = arr;
    EXPECT_EQ(l.push(p), 1);
    EXPECT_EQ(l.to_string(), "abc");
    EXPECT_EQ(l.push(p + 1), 1);
    EXPECT_EQ(l.to_string(), "bc");
    EXPECT_EQ(l.push(arr + 2), 1);
    EXPECT_EQ(l.to_string(), "c");

    auto p1 = arr;  // const char*
    EXPECT_EQ(l.push(p1), 1);
    EXPECT_EQ(l.to_string(), "abc");

    // unsupported types for string:
    // auto p2 = &arr;  // const char(*)[3]
    // l.push(p2);
  }

  // nullptr
  {
    l.settop(0);

    EXPECT_EQ(l.push(nullptr), 1);
    EXPECT_EQ(l.gettop(), 1);

    EXPECT_TRUE(l.isnil());

    EXPECT_EQ(l.push(std::nullptr_t{}), 1);
    EXPECT_TRUE(l.isnil());

    const std::nullptr_t cnullptr{};
    EXPECT_EQ(l.push(cnullptr), 1);
    EXPECT_TRUE(l.isnil());
  }

  // pair
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::make_pair(1, 2.5)), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::pair<int, float>>()), (std::pair<int, float>(1, 2.5)));
    EXPECT_EQ((l.to<std::pair<bool, int>>()), (std::pair<bool, int>(true, 2)));

    EXPECT_EQ(l.push(std::make_pair(std::make_pair(1, "s"), 2.5)), 1);
    EXPECT_EQ((l.to<std::pair<std::pair<int, std::string>, float>>()),
              (std::pair<std::pair<int, std::string>, float>({1, "s"}, 2.5)));
  }

  // array
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::array<int, 3>{1, 2, 3}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3}));
    // TODO
    // EXPECT_EQ((l.to<std::array<int, 3>>()), (std::array<int, 3>{1, 2, 3}));
  }

  // vector
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::vector<int>{1, 2, 3, 4}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // deque
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::deque<int>{1, 2, 3, 4}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // forward_list
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::forward_list<int>{1, 2, 3, 4}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // list
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::list<int>{1, 2, 3, 4}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ(l.to<std::vector<int>>(), (std::vector<int>{1, 2, 3, 4}));
  }

  // set
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::set<int>{1, 2, 3}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::map<int, bool>>()),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));
  }

  // unordered_set
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::unordered_set<int>{3, 1, 2}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::map<int, bool>>()),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));
  }

  // map
  {
    l.settop(0);

    EXPECT_EQ(l.push(std::map<int, bool>{{1, true}, {2, true}, {3, true}}), 1);
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::map<int, bool>>()),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));

    EXPECT_EQ(l.push(std::map<int, std::string>{
                  {1, "one"}, {2, "two"}, {3, "three"}}),
              1);
    EXPECT_EQ(
        (l.to<std::map<int, std::string>>()),
        (std::map<int, std::string>{{1, "one"}, {2, "two"}, {3, "three"}}));
  }

  // unordered_map
  {
    l.settop(0);
    EXPECT_EQ(l.push(std::unordered_map<int, std::string>{
                  {1, "one"}, {2, "two"}, {3, "three"}}),
              1);
    EXPECT_EQ(
        (l.to<std::map<int, std::string>>()),
        (std::map<int, std::string>{{1, "one"}, {2, "two"}, {3, "three"}}));
  }

  // complex
  {
    l.settop(0);
    EXPECT_EQ(l.push(std::make_pair(std::vector<int>{1, 2, 3},
                                    std::unordered_map<int, std::string>{
                                        {1, "one"}, {2, "two"}, {3, "three"}})),
              1);
    EXPECT_EQ((l.to<std::pair<std::vector<int>, std::map<int, std::string>>>()),
              (std::make_pair(std::vector<int>{1, 2, 3},
                              std::map<int, std::string>{
                                  {1, "one"}, {2, "two"}, {3, "three"}})));
  }
}

TEST(push, tuple) {
  luaw l;
  auto t = std::make_tuple(1, 2.5, std::string("str"));
  EXPECT_EQ(l.push(t), 1);
  EXPECT_EQ(l.gettop(), 1);

  EXPECT_EQ(l.seek(1).to_int(), 1);
  EXPECT_EQ(l.gettop(), 2);
  l.pop();

  EXPECT_EQ(l.seek(2).to_double(), 2.5);
  EXPECT_EQ(l.gettop(), 2);
  l.pop();

  EXPECT_EQ(l.seek(3).to_string(), "str");
  EXPECT_EQ(l.gettop(), 2);
  l.cleartop();

  EXPECT_EQ(l.push(std::tuple<>{}), 1);
  EXPECT_EQ(l.gettop(), 1);
  l.cleartop();

  // nested tuple
  auto t2 = std::make_tuple(2, std::vector<int>{1, 2}, t);
  EXPECT_EQ(l.push(t2), 1);
  EXPECT_EQ(l.seek(3).to<decltype(t)>(), t);
  EXPECT_EQ(l.seek(2).to_float(), 2.5);
  EXPECT_EQ(l.push(std::tuple<int, const std::tuple<bool, double>, float>{}),
            1);

  l.cleartop();

  const std::tuple<int, bool> t3;
  EXPECT_EQ(l.push(t3), 1);

  const auto &tr = t3;
  EXPECT_EQ(l.push(tr), 1);
  EXPECT_EQ(l.push(std::move(t3)), 1);
}

void func(int) {}

TEST(push, function) {
  luaw l;

  EXPECT_EQ(l.push(func), 1);
  EXPECT_EQ(l.push(&func), 1);
  EXPECT_EQ(l.push<luaw::function_tag>(func), 1);
  EXPECT_EQ(l.push<luaw::function_tag>(&func), 1);
  EXPECT_EQ(l.push<void(int)>(func), 1);
  EXPECT_EQ(l.push<void(int)>(&func), 1);

  // l.push<void (*)(int)>(func); // error with G++?
  // l.push<void (*)(int)>(&func);

  std::function<void(int)> f = func;
  EXPECT_EQ(l.push(f), 1);
  EXPECT_EQ(l.push(std::move(f)), 1);
  EXPECT_EQ(l.push<luaw::function_tag>(f), 1);
  EXPECT_EQ(l.push<luaw::function_tag>(std::move(f)), 1);

  auto l1 = [](int i) {};
  EXPECT_EQ(l.push(l1), 1);
  EXPECT_EQ(l.push([](int i) {}), 1);
  EXPECT_EQ(l.push<void(int)>([](int i) {}), 1);
  EXPECT_EQ(l.push<luaw::function_tag>([](int i) {}), 1);

  auto l2 = [](auto x) { return x; };
  EXPECT_EQ(l.push<int(int)>(l2), 1);
  EXPECT_EQ(l.push<double(double)>(l2), 1);
  EXPECT_EQ(l.push<int(double)>(l2), 1);
  EXPECT_EQ(l.push<double(int)>(l2), 1);

  // variadic function
  // l.push(printf); // error
}

TEST(push, newtable) {
  luaw l;
  l.push(luaw::newtable_tag{});
  EXPECT_TRUE(l.to<std::vector<int>>().empty());
}

TEST(push, lightuserdata) {
  luaw  l;
  int   x = 0;
  void *p = (void *)&x;
  EXPECT_EQ(l.push(p), 1);
}

TEST(push, nullptr) {
  luaw l;

  l.push(nullptr);
  EXPECT_TRUE(l.isnil());

  const volatile std::nullptr_t n = 0;
  l.push(n);
  EXPECT_TRUE(l.isnil());
}
