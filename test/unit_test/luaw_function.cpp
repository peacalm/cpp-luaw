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

struct Obj {
  int i = 1;
  Obj() {}
  Obj(const Obj &r) : i(r.i) {}
  Obj(Obj &&r) : i(r.i) { r.i = -1; }
  ~Obj() {}
};

TEST(luaw_function, type_check) {
  (void)luaw::function<void(int)>{};
  (void)luaw::function<void(const int)>{};
  // (void)luaw::function<void( int &)>{}; // error
  (void)luaw::function<void(const int &)>{};
  (void)luaw::function<void(int &&)>{};
  (void)luaw::function<void(const int &&)>{};

  (void)luaw::function<int()>{};
  // (void)luaw::function<int &()>{}; // error
  // (void)luaw::function<int && ()>{};  // error
  (void)luaw::function<std::tuple<int, int>()>{};
  // (void)luaw::function<std::tuple<int &, int>()>{}; // error

  (void)luaw::function<void(luaw::luavalueref)>{};
  (void)luaw::function<void(const luaw::luavalueref &)>{};
  (void)luaw::function<void(luaw::luavalueref &&)>{};

  (void)luaw::function<luaw::luavalueref()>{};
  // (void)luaw::function<luaw::luavalueref &()>{}; // error
  // (void)luaw::function<const luaw::luavalueref &()>{}; // error

  (void)luaw::function<luaw::luavalueref(luaw::luavalueidx)>{};
}

TEST(luaw_function, arg_can_copy_can_move) {
  luaw l;
  EXPECT_EQ(l.dostring("function echo(o) return o; end"), LUA_OK);

  {
    Obj o;
    // Will get a copy of o
    auto echo = l.get<luaw::function<luaw::luavalueref(const Obj &)>>("echo");
    auto ref  = echo(o);
    EXPECT_FALSE(echo.failed());
    l.push(ref);
    Obj *p = l.to<Obj *>(-1);
    EXPECT_NE(p, &o);
    l.cleartop();
  }
  {
    Obj o;
    // Will get a copy of the pointer
    auto echo = l.get<luaw::function<luaw::luavalueref(Obj *)>>("echo");
    auto ref  = echo(&o);
    EXPECT_FALSE(echo.failed());
    l.push(ref);
    Obj *p = l.to<Obj *>(-1);
    EXPECT_EQ(p, &o);
    l.cleartop();
  }
  {
    Obj a;
    EXPECT_EQ(a.i, 1);
    Obj b = std::move(a);
    EXPECT_EQ(b.i, 1);
    EXPECT_EQ(a.i, -1);
  }
  {
    // Move o
    Obj o;
    o.i = 123;
    EXPECT_EQ(o.i, 123);
    auto echo = l.get<luaw::function<luaw::luavalueref(Obj &&)>>("echo");
    auto ref  = echo(std::move(o));
    EXPECT_FALSE(echo.failed());
    EXPECT_EQ(o.i, -1);

    l.push(ref);
    Obj *p = l.to<Obj *>(-1);
    EXPECT_EQ(p->i, 123);
    EXPECT_NE(p, &o);
    l.cleartop();
  }
  {
    // Move o
    Obj o;
    o.i = 123;
    EXPECT_EQ(o.i, 123);
    auto echo = l.get<luaw::function<luaw::luavalueref(Obj)>>("echo");
    auto ref  = echo(std::move(o));
    EXPECT_FALSE(echo.failed());
    EXPECT_EQ(o.i, -1);

    l.push(ref);
    Obj *p = l.to<Obj *>(-1);
    EXPECT_EQ(p->i, 123);
    EXPECT_NE(p, &o);
    l.cleartop();
  }
}

TEST(luaw_function, arg_ref) {
  luaw l;
  EXPECT_EQ(l.dostring("function echo(o) return o; end"), LUA_OK);

  // perfect echo
  auto echo =
      l.get<luaw::function<luaw::luavalueref(luaw::luavalueref)>>("echo");

  l.set("o", Obj{});
  auto oref = l.get<luaw::luavalueref>("o");
  auto ret  = echo(oref);
  EXPECT_FALSE(echo.failed());

  l.push(oref);
  Obj *a = l.to<Obj *>(-1);
  l.push(ret);
  Obj *b = l.to<Obj *>(-1);
  l.cleartop();

  // same obj
  EXPECT_EQ(a, b);

  l.cleartop();
}

TEST(luaw_function, no_result) {
  luaw l;
  EXPECT_EQ(l.dostring("function f(o) end"), LUA_OK);

  auto f   = l.get<luaw::function<int(int)>>("f");
  int  ret = f(1);
  EXPECT_TRUE(f.failed());
  EXPECT_TRUE(!f.result_exists());
  EXPECT_TRUE(!f.result_enough());
  EXPECT_EQ(f.expected_result_size(), 1);
  EXPECT_EQ(f.real_result_size(), 0);
}

TEST(luaw_function, result_not_enough) {
  luaw l;
  EXPECT_EQ(l.dostring("function f(o) return o; end"), LUA_OK);

  auto f   = l.get<luaw::function<std::tuple<int, int>(int)>>("f");
  auto ret = f(1);
  EXPECT_TRUE(f.failed());
  EXPECT_TRUE(f.result_exists());
  EXPECT_TRUE(!f.result_enough());
  EXPECT_TRUE(!f.result_failed());  // not fail, just not enough
  EXPECT_EQ(f.expected_result_size(), 2);
  EXPECT_EQ(f.real_result_size(), 1);

  EXPECT_EQ(std::get<0>(ret), 1);
}

TEST(luaw_function, expect_0_result) {
  luaw l;
  EXPECT_EQ(l.dostring("function f(o) return o; end"), LUA_OK);
  {
    auto f   = l.get<luaw::function<std::tuple<>(int)>>("f");
    auto ret = f(1);
    EXPECT_EQ(std::tuple_size<decltype(ret)>(), 0);
    EXPECT_TRUE(!f.failed());

    EXPECT_TRUE(f.result_enough());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.result_failed());

    EXPECT_EQ(f.expected_result_size(), 0);
    EXPECT_EQ(f.real_result_size(), 1);
  }
  {
    auto f = l.get<luaw::function<void(int)>>("f");
    f(1);
    EXPECT_TRUE(!f.failed());

    EXPECT_TRUE(f.result_enough());
    EXPECT_TRUE(f.result_exists());
    EXPECT_FALSE(f.result_failed());

    EXPECT_EQ(f.expected_result_size(), 0);
    EXPECT_EQ(f.real_result_size(), 1);
  }
}

TEST(luaw_function, modify_arg) {
  luaw l;
  l.register_member("i", &Obj::i);

  int retcode = l.dostring(R"(
    function f(o)
      o.i = o.i + 100
      return o
    end
  )");

  {
    auto f = l.get<peacalm::luaw::function<Obj(Obj)>>("f");
    Obj  o;
    EXPECT_EQ(o.i, 1);
    Obj ret = f(o);
    EXPECT_FALSE(f.failed());
    EXPECT_EQ(o.i, 1);
    EXPECT_EQ(ret.i, 101);
  }

  {
    auto f = l.get<peacalm::luaw::function<Obj(Obj *)>>("f");
    Obj  o;
    EXPECT_EQ(o.i, 1);
    Obj ret = f(&o);
    EXPECT_FALSE(f.failed());
    EXPECT_EQ(o.i, 101);
    EXPECT_EQ(ret.i, 101);
  }
}

TEST(luaw_function, map_as_table) {
  luaw l;
  int  retcode = l.dostring(R"(
    function f(o)
      o.a = 1
      o.b = 2
      return o
    end
  )");
  EXPECT_EQ(retcode, LUA_OK);

  using map_t = std::map<std::string, int>;

  auto f = l.get<peacalm::luaw::function<map_t(map_t &&)>>("f");

  map_t m{{"b", -1}, {"c", -1}};

  // Won't move to full userdata, because will construct a table by m.
  // So, m will not be cleared.
  auto ret = f(std::move(m));

  EXPECT_FALSE(f.failed());

  EXPECT_EQ(m.size(), 2);
  EXPECT_EQ(m["a"], 0);
  EXPECT_EQ(m["b"], -1);
  EXPECT_EQ(m["c"], -1);

  EXPECT_EQ(ret.size(), 3);
  EXPECT_EQ(ret["a"], 1);
  EXPECT_EQ(ret["b"], 2);
  EXPECT_EQ(ret["c"], -1);
}

}  // namespace
