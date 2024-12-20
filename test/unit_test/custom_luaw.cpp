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

struct dummy_provider {
  int def = 0;
  dummy_provider(int i = 1) : def(i) {}
  bool provide(luaw &l, const char *vname) {
    l.push(def);
    return true;
  }
};

struct bad_provider {
  // error: pushes nothing
  bool provide(luaw &l, const char *vname) { return true; }
};

struct bad_provider2 {
  // error: pushes more than 1 value
  bool provide(luaw &l, const char *vname) {
    l.push(0);
    l.push(1);
    return true;
  }
};

struct bad_provider3 {
  // error: returns false
  bool provide(luaw &l, const char *vname) {
    l.push(0);
    return false;
  }
};

TEST(custom_luaw, eval) {
  custom_luaw<std::unique_ptr<dummy_provider>> l;
  l.provider(std::make_unique<dummy_provider>());
  EXPECT_EQ(l.eval_int("return a + b"), 2);
  l.provider(std::make_unique<dummy_provider>(2));
  EXPECT_EQ(l.eval_int("return x"), 2);
  EXPECT_EQ(l.eval_int("return a + b"), 4);
  EXPECT_EQ(l.eval_int("a = 4; return a + b"), 6);
}

TEST(custom_luaw, eval_failed) {
  {
    custom_luaw<std::unique_ptr<dummy_provider>> l;
    bool                                         failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_luaw<std::unique_ptr<dummy_provider>> l;
    l.provider(std::make_unique<dummy_provider>());
    lua_pushlightuserdata(l.L(), (void *)0);
    lua_setfield(l.L(), LUA_REGISTRYINDEX, "this");
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_luaw<std::unique_ptr<bad_provider>> l;
    l.provider(std::make_unique<bad_provider>());
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_luaw<std::unique_ptr<bad_provider2>> l;
    l.provider(std::make_unique<bad_provider2>());
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
  {
    custom_luaw<std::unique_ptr<bad_provider3>> l;
    l.provider(std::make_unique<bad_provider3>());
    bool failed;
    EXPECT_EQ(l.eval_int("return a + b", 0, false, &failed), 0);
    EXPECT_TRUE(failed);
  }
}

TEST(custom_luaw, move_ctor) {
  {
    custom_luaw<std::unique_ptr<dummy_provider>> l;
    EXPECT_FALSE(l.provider());
    l.provider(std::make_unique<dummy_provider>());
    EXPECT_TRUE(l.provider());
    EXPECT_EQ(l.eval_int("return a + b"), 2);

    auto l2(std::move(l));
    EXPECT_TRUE(l.L() == nullptr);
    EXPECT_FALSE(l.provider());
    EXPECT_TRUE(l2.provider());
    EXPECT_EQ(l2.eval_int("return a + b"), 2);

    custom_luaw<std::unique_ptr<dummy_provider>> l3;
    l3 = std::move(l2);
    EXPECT_TRUE(l2.L() == nullptr);
    EXPECT_FALSE(l2.provider());
    EXPECT_TRUE(l3.provider());
    EXPECT_EQ(l3.eval_int("return a + b"), 2);
  }
  {
    custom_luaw<std::shared_ptr<dummy_provider>> l;
    EXPECT_FALSE(l.provider());
    l.provider(std::make_shared<dummy_provider>());
    EXPECT_TRUE(l.provider());
    EXPECT_EQ(l.eval_int("return a + b"), 2);

    auto l2(std::move(l));
    EXPECT_TRUE(l.L() == nullptr);
    EXPECT_FALSE(l.provider());
    EXPECT_TRUE(l2.provider());
    EXPECT_EQ(l2.eval_int("return a + b"), 2);

    custom_luaw<std::shared_ptr<dummy_provider>> l3;
    l3 = std::move(l2);
    EXPECT_TRUE(l2.L() == nullptr);
    EXPECT_FALSE(l2.provider());
    EXPECT_TRUE(l3.provider());
    EXPECT_EQ(l3.eval_int("return a + b"), 2);
  }
  {
    custom_luaw<dummy_provider *> l;
    EXPECT_FALSE(l.provider());
    dummy_provider *p = new dummy_provider;
    l.provider(p);
    EXPECT_TRUE(l.provider());
    EXPECT_EQ(l.eval_int("return a + b"), 2);

    auto l2(std::move(l));
    EXPECT_TRUE(l.L() == nullptr);
    EXPECT_TRUE(l.provider());  // not clear if not smart pointer type
    EXPECT_TRUE(l2.provider());
    EXPECT_EQ(l2.eval_int("return a + b"), 2);

    custom_luaw<dummy_provider *> l3;
    l3 = std::move(l2);
    EXPECT_TRUE(l2.L() == nullptr);
    EXPECT_TRUE(l2.provider());  // not clear if not smart pointer type
    EXPECT_TRUE(l3.provider());
    EXPECT_EQ(l3.eval_int("return a + b"), 2);

    delete p;
  }
}

}  // namespace
