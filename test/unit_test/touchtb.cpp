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

TEST(touchtb, touchtb) {
  lua_wrapper l;
  l.gtouchtb("g");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.istable());

  l.gtouchtb("g");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.istable());

  l.touchtb("gg");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_TRUE(l.istable());

  l.pop();
  l.touchtb("gg");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_TRUE(l.istable());

  l.settop(0);
}

TEST(setfield, setfield) {
  lua_wrapper l;
  l.gseek_env().touchtb("g");

  EXPECT_EQ(l.gettop(), 2);

  l.setfield("a", 1);
  l.setfield("b", 2);
  l.setfield(1, "one");
  l.setfield<lua_wrapper::function_tag>("fadd",
                                        [](int a, int b) { return a + b; });

  EXPECT_EQ(l.gettop(), 2);

  EXPECT_EQ(l.get<int>({"g", "a"}), 1);
  EXPECT_EQ(l.get<int>({"g", "b"}), 2);
  EXPECT_EQ(l.gseek("g").seek(1).to<std::string>(), "one");

  auto fadd = l.get<lua_wrapper::function<int(int, int)>>({"g", "fadd"});
  EXPECT_EQ(fadd(1, 1), 2);

  auto f = l.get<lua_wrapper::function<int(int, int)>>({"g", "f"});
  EXPECT_EQ(f(1, 1), 0);
  EXPECT_TRUE(f.failed());

  auto ff = l.get<lua_wrapper::function<int(int, int)>>({"g", "f", "ff"});
  EXPECT_EQ(ff(1, 1), 0);
  EXPECT_TRUE(ff.failed());

  l.cleartop();
  EXPECT_EQ(l.gettop(), 0);

  l.gseek_env();
  std::vector<int> x{1, 2, 3};
  l.setfield("x", x);
  EXPECT_EQ(l.get<std::vector<int>>("x"), x);

  l.set({"g", "gg"}, 1);
  EXPECT_EQ(l.get_int({"g", "gg"}), 1);

  l.set({"g", "gg", "x"}, 1);
  l.set({"g", "gg", "y"}, 2);
  l.set<double(double, double)>({"g", "gg", "fadd"},
                                [](auto x, auto y) { return x + y; });
  EXPECT_EQ(l.get<int>({"g", "gg", "x"}), 1);
  EXPECT_EQ(l.get<int>({"g", "gg", "y"}), 2);
  EXPECT_EQ(l.get<std::function<double(double, double)>>({"g", "gg", "fadd"})(
                1.5, 1.5),
            3);
  EXPECT_EQ(l.eval<double>("return g.gg.fadd(1.25, 1.5) "), 2.75);
  EXPECT_EQ(l.get<std::function<int(int, int)>>({"g", "gg", "fadd"})(1, 1), 2);
}
