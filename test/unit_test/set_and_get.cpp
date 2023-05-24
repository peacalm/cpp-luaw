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

TEST(set_and_get, simple_types) {
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

TEST(set_and_get, template_get) {
  lua_wrapper l;
  bool        failed, exists;
  l.dostring(
      "a = 1; b = {1,2}; c={'x', 'y', 'z'}; d={a=1,b=2}; e={ {a=1,b=2}, "
      "{a=2,b=1} }");
  EXPECT_EQ(l.get<long>("a"), 1);
  EXPECT_EQ(l.get<long>("a", false, &failed, &exists), 1);
  EXPECT_FALSE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ((l.get<std::pair<long, long>>("b")), (std::pair<long, long>{1, 2}));

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

TEST(set_and_get, recursive_get) {
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

TEST(set_and_get, template_set) {
  lua_wrapper l;
  l.set("a", 1);
  EXPECT_EQ(l.get<int>("a"), 1);

  l.set("a", nullptr);
  l.gseek("a");
  EXPECT_TRUE(l.isnil());

  bool failed, exists;
  int  a = l.get_int("a", -1, false, &failed, &exists);
  EXPECT_EQ(a, -1);
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);

  {
    // Note: C++ std::set -> Lua Key-True table
    auto x = std::set<int>{1, 2, 3};
    l.set("x", x);
    EXPECT_EQ((l.get<std::map<int, bool>>("x")),
              (std::map<int, bool>{{1, true}, {2, true}, {3, true}}));
  }
  {
    auto x = std::map<std::string, std::vector<int>>{{"odd", {1, 3, 5, 7}},
                                                     {"even", {2, 4, 6, 8}}};
    l.set("x", x);
    EXPECT_EQ(l.get<decltype(x)>("x"), x);
  }
  {
    auto x =
        std::make_pair(std::vector<std::vector<double>>{{1.1, 2.2}, {0.1, 0.2}},
                       std::map<std::string, std::vector<int>>{
                           {"odd", {1, 3, 5, 7}}, {"even", {2, 4, 6, 8}}});
    l.set("x", x);
    EXPECT_EQ(l.get<decltype(x)>("x"), x);
  }
  {
    auto x = std::vector<int>{1, 2, 3};
    l.set<decltype(x)>("x", x);
    EXPECT_EQ(l.get<decltype(x)>("x"), x);
  }
  {
    const auto x = std::vector<int>{1, 2, 3};
    l.set<std::vector<int>>("x", x);
    EXPECT_EQ(l.get<std::vector<int>>("x"), x);
  }
  {
    const auto x = std::vector<int>{1, 2, 3};
    l.set<decltype(x)>("x", x);
    EXPECT_EQ(l.get<decltype(x)>("x"), x);
  }
  {
    const auto &&x = std::vector<int>{1, 2, 3};
    l.set<decltype(x)>("x", x);
    EXPECT_EQ(l.get<const std::vector<int>>("x"), x);
  }
}

int echo(int i) {
  printf("echo %d\n", i);
  return i;
}

struct Callable {
  int w1_ = 100, w2_ = 1;

  Callable(int w1 = 100, int w2 = 1) : w1_(w1), w2_(w2) {
    printf("Callable(%d, %d)\n", w1_, w2_);
  }

  Callable(const Callable &r) : w1_(r.w1_), w2_(r.w2_) {
    printf("Callable(const Callable &)(%d, %d)\n", w1_, w2_);
  }

  Callable(Callable &&r) : w1_(r.w1_), w2_(r.w2_) {
    printf("Callable(Callable &&)(%d, %d)\n", w1_, w2_);
  }

  ~Callable() { printf("~Callable(%d, %d)\n", w1_, w2_); }

  int operator()(int a, int b) {
    int ret = a * w1_ + b * w2_;
    printf("call Callable::operator(%d, %d) = %d\n", a, b, ret);
    return ret;
  }
};

TEST(set_and_get, set_function) {
  lua_wrapper l;

  // C function
  {
    l.set("f1", echo);
    EXPECT_EQ(l.eval_int("return f1(1)"), 1);
    l.set("f1", nullptr);
    EXPECT_EQ(l.eval_int("return f1(1)"), 0);

    l.set("f2", &echo);
    EXPECT_EQ(l.eval_int("return f2(2)"), 2);

    l.set<lua_wrapper::function_tag>("f3", echo);
    EXPECT_EQ(l.eval_int("return f3(3)"), 3);

    l.set<lua_wrapper::function_tag>("f4", &echo);
    EXPECT_EQ(l.eval_int("return f4(4)"), 4);
  }

  // std::function
  {
    std::function<int(int, int)> fadd = [](int a, int b) { return a + b; };
    l.set("fadd", fadd);
    EXPECT_EQ(l.eval_int("return fadd(1, 1)"), 2);

    l.set<lua_wrapper::function_tag>("fadd2", fadd);
    EXPECT_EQ(l.eval_int("return fadd2(1, 2)"), 3);
  }

  // lambda
  {
    // captureless
    auto lmul = [](int a, int b) { return a * b; };
    l.set("lmul", lmul);
    EXPECT_EQ(l.eval_int("return lmul(1, 1)"), 1);

    l.set<lua_wrapper::function_tag>("lmul2", lmul);
    EXPECT_EQ(l.eval_int("return lmul2(2, 3)"), 6);

    // not captureless
    int  w      = 10;
    auto ltimes = [&](int x) { return x * w; };
    l.set<lua_wrapper::function_tag>("ltimes", ltimes);
    EXPECT_EQ(l.eval_int("return ltimes(5)"), 50);

    // generic lambda
    auto gadd = [](auto a, auto b) { return a + b; };
    l.set<int(int, int)>("gadd_int", gadd);
    EXPECT_EQ(l.eval_int("return gadd_int(2, 3)"), 5);

    l.set<double(double, double)>("gadd_double", gadd);
    EXPECT_EQ(l.eval_double("return gadd_double(1.25, 2.25)"), 3.5);

    // complex arg type
    auto push = [](std::vector<int> a, std::vector<int> b) -> std::vector<int> {
      for (int i : b) a.push_back(i);
      return a;
    };
    l.set("lmerge", push);
    EXPECT_EQ(l.eval<std::vector<int>>("return lmerge({1,2,3}, {11,22})"),
              (std::vector<int>{1, 2, 3, 11, 22}));
    l.set<lua_wrapper::function_tag>("lmerge", push);
    EXPECT_EQ(l.eval<std::vector<int>>("return lmerge({1,2,3}, {11,22})"),
              (std::vector<int>{1, 2, 3, 11, 22}));
  }

  // custom object as function
  {
    l.set<lua_wrapper::function_tag>("c", Callable{});
    EXPECT_EQ(l.eval_int("return c(1, 5)"), 105);
    EXPECT_EQ(l.eval_int("return c(false, true)"), 1);
    EXPECT_EQ(l.eval_int("return c('a', 'b')"), 0);

    Callable c(50, 3);
    l.set<lua_wrapper::function_tag>("c2", c);
    EXPECT_EQ(l.eval_int("return c2(2, 1)"), 103);
  }
  {
    Callable c(11111, 11111);
    l.set_nil("c");
    l.set_nil("c2");
    EXPECT_EQ(l.eval_int("return c2(2, 1)"), 0);
  }
  {
    Callable c(0, 0);
    l.reset();  // release "c" and "c2"
  }
}
