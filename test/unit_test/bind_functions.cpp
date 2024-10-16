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

#include "main.h"

namespace {

int echo(int i) {
  printf("echo %d\n", i);
  return i;
}

TEST(bind_functions, c_function) {
  luaw l;

  l.set("f1", echo);
  EXPECT_EQ(l.eval_int("return f1(1)"), 1);
  l.set("f1", nullptr);
  EXPECT_EQ(l.eval_int("return f1(1)"), 0);

  l.set("f2", &echo);
  EXPECT_EQ(l.eval_int("return f2(2)"), 2);

  l.set<luaw::function_tag>("f3", echo);
  EXPECT_EQ(l.eval_int("return f3(3)"), 3);

  l.set<luaw::function_tag>("f4", &echo);
  EXPECT_EQ(l.eval_int("return f4(4)"), 4);

  EXPECT_EQ(l.gettop(), 0);
}

template <typename T>
T tadd(T a, T b) {
  return a + b;
}

TEST(bind_functions, cpp_template_function) {
  luaw l;

  l.set("tadd", tadd<int>);
  EXPECT_EQ(l.eval_int("return tadd(1, 1)"), 2);

  l.set<double(double, double)>("tadd2", tadd);
  EXPECT_EQ(l.eval_int("return tadd2(1.5, 1.5)"), 3);
}

TEST(bind_functions, std_function) {
  luaw l;

  std::function<int(int, int)> fadd = [](int a, int b) { return a + b; };
  l.set("fadd", fadd);
  EXPECT_EQ(l.eval_int("return fadd(1, 1)"), 2);

  l.set<luaw::function_tag>("fadd2", fadd);
  EXPECT_EQ(l.eval_int("return fadd2(1, 2)"), 3);

  EXPECT_EQ(l.gettop(), 0);
}

TEST(bind_functions, lambda) {
  luaw l;
  // captureless
  auto lmul = [](int a, int b) { return a * b; };
  l.set("lmul", lmul);
  EXPECT_EQ(l.eval_int("return lmul(1, 1)"), 1);

  l.set<luaw::function_tag>("lmul2", lmul);
  EXPECT_EQ(l.eval_int("return lmul2(2, 3)"), 6);

  // not captureless
  int  w      = 10;
  auto ltimes = [&](int x) { return x * w; };
  l.set<luaw::function_tag>("ltimes", ltimes);
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
  l.set<luaw::function_tag>("lmerge", push);
  EXPECT_EQ(l.eval<std::vector<int>>("return lmerge({1,2,3}, {11,22})"),
            (std::vector<int>{1, 2, 3, 11, 22}));

  EXPECT_EQ(l.gettop(), 0);
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

TEST(bind_functions, custom_objects_as_function) {
  luaw l;
  {
    l.set<luaw::function_tag>("c", Callable{});
    EXPECT_EQ(l.eval_int("return c(1, 5)"), 105);
    EXPECT_EQ(l.eval_int("return c(false, true)"), 1);
    EXPECT_EQ(l.eval_int("return c('a', 'b')"), 0);

    Callable c(50, 3);
    l.set<luaw::function_tag>("c2", c);
    EXPECT_EQ(l.eval_int("return c2(2, 1)"), 103);

    EXPECT_EQ(l.gettop(), 0);
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

TEST(bind_functions, function_return_tuple) {
  luaw l;
  l.set("f", [](double n) { return std::make_tuple(true, n * n, n * n * n); });
  int retcode =
      l.dostring("a, b, c = f(3); assert(a == true and b == 9 and c == 27)");
  EXPECT_EQ(retcode, LUA_OK);
  EXPECT_EQ(l.gettop(), 0);

  int n = 2;
  l.set<luaw::function_tag>(
      "f", [&]() { return std::make_tuple(true, n * n, n * n * n); });
  bool failed;
  auto fret = l.eval<std::tuple<bool, int, int>>("return f()", false, &failed);
  EXPECT_FALSE(failed);
  EXPECT_EQ(std::get<0>(fret), true);
  EXPECT_EQ(std::get<1>(fret), 4);
  EXPECT_EQ(std::get<2>(fret), 8);
  EXPECT_EQ(l.gettop(), 0);

  auto t = l.eval<std::tuple<bool, double, double>>("return f()");
  EXPECT_EQ(t, std::make_tuple(true, 4.0, 8.0));
  EXPECT_EQ(l.gettop(), 0);
}

std::tuple<int, int> point(int x = 3, int y = 4) {
  return std::make_tuple(x, y);
}

TEST(bind_functions, function_with_default_arguments) {
  luaw l;
  l.set("point", point);

  l.dostring("x1, y1 = point(1, 2)");
  EXPECT_EQ(l.get_int("x1"), 1);
  EXPECT_EQ(l.get_int("y1"), 2);

  l.dostring("x2, y2 = point(1)");
  EXPECT_EQ(l.get_int("x2"), 1);
  EXPECT_EQ(l.get_int("y2"), 0);  // not 4

  l.dostring("x3, y3 = point()");
  EXPECT_EQ(l.get_int("x3"), 0);  // not 3
  EXPECT_EQ(l.get_int("y3"), 0);  // not 4

  // also for lambda
  auto f = [](int x = 3, int y = 4) { return std::make_tuple(x, y); };
  l.set("f", f);
  l.dostring("x4, y4 = f(1)");
  EXPECT_EQ(l.get_int("x4"), 1);
  EXPECT_EQ(l.get_int("y4"), 0);
}

int noexcept_add(int x, int y) noexcept { return x + y; }

TEST(bind_functions, function_with_noexcept) {
  luaw l;
  l.set("add", noexcept_add);
  EXPECT_EQ(l.eval<int>("return add(1, 2)"), 3);

  auto mul = [](int a, int b) noexcept { return a * b; };
  l.set("mul", mul);
  EXPECT_EQ(l.eval<int>("return mul(2, 3)"), 6);
}

}  // namespace
