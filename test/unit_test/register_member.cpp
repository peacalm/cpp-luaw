// Copyright (c) 2023 Li Shuangquan. All Rights Reserved.
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
  Obj() {}
  Obj(int v) : i(v) {}
  Obj(int v, int cv) : i(v), ci(cv) {}

  int                           i  = 1;
  const int                     ci = 1;
  volatile int                  vi = 1;
  std::map<std::string, double> dm;

  int geti() const { return i; }
  int cv_geti() const volatile { return i; }

  int plus() { return ++i; }
  int plusby(int d) {
    i += d;
    return i;
  }

  void reset() { i = 0; }

  int overloaded_f() { return i + 1000; }
  int overloaded_f() const { return i + 2000; }
  int overloaded_f() volatile { return i + 3000; }
  int overloaded_f() const volatile { return i + 4000; }
  int overloaded_f(int d) { return i + d; }

  void referenced_fl() & {}
  void referenced_fr() && {}
};

}  // namespace

TEST(register_member, register_ctor) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);

  l.register_ctor<Obj()>("NewObj");
  EXPECT_EQ(l.dostring("a = NewObj()"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return a.i"), 1);
  EXPECT_EQ(l.eval<int>("a.i = 2; return a.i"), 2);
  EXPECT_EQ(l.get<int>({"a", "i"}), 2);
  EXPECT_EQ(l.get<int>({"a", "ci"}), 1);

  l.register_ctor<std::add_const_t<Obj>()>("NewConstObj");
  EXPECT_EQ(l.dostring("b = NewConstObj()"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), 1);
  EXPECT_NE(l.dostring("b.i = 2"), LUA_OK);
  l.log_error_out();
  EXPECT_EQ(l.get<int>({"b", "i"}), 1);
  EXPECT_EQ(l.get<int>({"b", "ci"}), 1);

  // another ctor
  l.register_ctor<Obj(int)>("NewObj1");
  EXPECT_EQ(l.dostring("a = NewObj1(3)"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return a.i"), 3);
  EXPECT_EQ(l.eval<int>("a.i = 2; return a.i"), 2);
  EXPECT_EQ(l.get<int>({"a", "i"}), 2);
  EXPECT_EQ(l.get<int>({"a", "ci"}), 1);

  // another ctor
  l.register_ctor<Obj(int, int)>("NewObj2");  // constructor with 2 argument
  EXPECT_EQ(l.dostring("a = NewObj2(4, 5)"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return a.i"), 4);
  EXPECT_EQ(l.eval<int>("return a.ci"), 5);

  // l.register_ctor<Obj>("NewObj"); // error
  EXPECT_EQ(l.gettop(), 0);
}

int getmember(const Obj* o, const char* mname) { return 123; }

TEST(register_member, register_member_functions) {
  luaw l;

  l.register_member("plus", &Obj::plus);
  l.register_member<int (Obj::*)()>("plus", &Obj::plus);

  EXPECT_EQ(l.gettop(), 0);

  // l.register_member("overloaded_f", &Obj::overloaded_f);  // error
  l.register_member<int (Obj::*)()>("overloaded_f1", &Obj::overloaded_f);
  l.register_member<int (Obj::*)() const>("overloaded_f2", &Obj::overloaded_f);
  l.register_member<int (Obj::*)() volatile>("overloaded_f3",
                                             &Obj::overloaded_f);
  l.register_member<int (Obj::*)() const volatile>("overloaded_f4",
                                                   &Obj::overloaded_f);
  l.register_member<int (Obj::*)(int)>("overloaded_f5", &Obj::overloaded_f);

  // fake member functions
  // l.register_member("getmember", &getmember); // error
  l.register_member<int (Obj::*)(const char*)>("getmember1", &getmember);
  l.register_member<int (Obj::*)(const char*)>("getmember2", getmember);
  l.register_member<int (Obj::*)(const char*)>(
      "getmember3", std::function<decltype(getmember)>(getmember));
  l.register_member<int (Obj::*)(const char*)>(
      "getmember4", [](const Obj* o, const char* mname) { return 123; });

  // unsupported
  // l.register_member("referenced_fl", &Obj::referenced_fl);  // error
  // l.register_member("referenced_fr", &Obj::referenced_fr);  // error

  EXPECT_EQ(l.gettop(), 0);
}

TEST(register_member, member_variables) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);
  l.register_member("vi", &Obj::vi);

  EXPECT_EQ(l.gettop(), 0);

  {
    l.set("o", Obj{});

    l.gseek("o");
    EXPECT_TRUE(l.indexable());
    EXPECT_TRUE(!l.istable());
    EXPECT_EQ(l.seek("i").to_int(-1), 1);
    l.cleartop();

    EXPECT_EQ(l.eval_int("return o.i"), 1);
    EXPECT_EQ(l.eval_int("return o.ci"), 1);
    EXPECT_EQ(l.eval_int("return o.vi"), 1);

    EXPECT_EQ(l.get_int({"o", "i"}), 1);
    EXPECT_EQ(l.get_int({"o", "ci"}), 1);
    EXPECT_EQ(l.get_int({"o", "vi"}), 1);

    EXPECT_EQ(l.lget<int>({}, "o", "i"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "ci"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "vi"), 1);

    l.lset("o", "i", 2);
    EXPECT_EQ(l.get_int({"o", "i"}), 2);

    l.set({"o", "i"}, 3);
    EXPECT_EQ(l.eval_int("return o.i"), 3);

    l.dostring("o.i = 4");
    EXPECT_EQ(l.lget<int>({}, "o", "i"), 4);

    l.gseek("o");
    l.setkv("i", 5);
    EXPECT_EQ(l.get_int({"o", "i"}), 5);
    l.cleartop();

    l.set({"o", "vi"}, 6);
    EXPECT_EQ(l.eval_int("return o.vi"), 6);

    EXPECT_EQ(l.gettop(), 0);

    EXPECT_NE(l.dostring("o.ci = 7"), LUA_OK);  // won't work, for ci is const
    EXPECT_EQ(l.gettop(), 1);                   // has error msg
    l.log_error_in_stack();
    l.pop();

    EXPECT_EQ(l.get_int({"o", "ci"}), 1);  // ci is still 1

    bool failed;
    EXPECT_EQ(l.eval<int>("o.ci = 7; return o.ci", false, &failed), 0);
    EXPECT_TRUE(failed);

    EXPECT_DEATH(l.lset("o", "ci", 7), ".*Const member.*");

    EXPECT_EQ(l.gettop(), 0);
  }
  {
    // set pointer
    l.set_nil("o");
    EXPECT_EQ(l.get_int({"o", "i"}), 0);

    Obj o;
    l.set("o", &o);

    l.gseek("o");
    EXPECT_TRUE(l.indexable());
    EXPECT_EQ(l.seek("i").to_int(), 1);
    l.cleartop();

    EXPECT_EQ(l.eval_int("return o.i"), 1);
    EXPECT_EQ(l.eval_int("return o.ci"), 1);
    EXPECT_EQ(l.eval_int("return o.vi"), 1);

    EXPECT_EQ(l.get_int({"o", "i"}), 1);
    EXPECT_EQ(l.get_int({"o", "ci"}), 1);
    EXPECT_EQ(l.get_int({"o", "vi"}), 1);

    EXPECT_EQ(l.lget<int>({}, "o", "i"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "ci"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "vi"), 1);

    l.lset("o", "i", 2);
    EXPECT_EQ(l.get_int({"o", "i"}), 2);
    EXPECT_EQ(o.i, 2);

    l.set({"o", "i"}, 3);
    EXPECT_EQ(l.eval_int("return o.i"), 3);
    EXPECT_EQ(o.i, 3);

    l.dostring("o.i = 4");
    EXPECT_EQ(l.lget<int>({}, "o", "i"), 4);
    EXPECT_EQ(o.i, 4);

    l.gseek("o");
    l.setkv("i", 5);
    EXPECT_EQ(l.get_int({"o", "i"}), 5);
    EXPECT_EQ(o.i, 5);
    l.cleartop();

    l.set({"o", "vi"}, 6);
    EXPECT_EQ(l.eval_int("return o.vi"), 6);
    EXPECT_EQ(o.vi, 6);

    EXPECT_EQ(l.gettop(), 0);

    EXPECT_NE(l.dostring("o.ci = 7"), LUA_OK);  // won't work, for ci is const
    EXPECT_EQ(l.gettop(), 1);                   // has error msg
    l.log_error_in_stack();
    l.pop();

    EXPECT_EQ(l.get_int({"o", "ci"}), 1);  // ci is still 1

    bool failed;
    EXPECT_EQ(l.eval<int>("o.ci = 7; return o.ci", false, &failed), 0);
    EXPECT_TRUE(failed);

    EXPECT_DEATH(l.lset("o", "ci", 7), ".*Const member.*");

    EXPECT_EQ(l.gettop(), 0);
  }
  {
    l.set("o", nullptr);
    EXPECT_EQ(l.get_int({"o", "i"}), 0);

    l.set("o", std::add_const_t<Obj>{});

    EXPECT_EQ(l.eval_int("return o.i"), 1);
    EXPECT_EQ(l.eval_int("return o.ci"), 1);
    EXPECT_EQ(l.eval_int("return o.vi"), 1);

    EXPECT_EQ(l.get_int({"o", "i"}), 1);
    EXPECT_EQ(l.get_int({"o", "ci"}), 1);
    EXPECT_EQ(l.get_int({"o", "vi"}), 1);

    EXPECT_EQ(l.lget<int>({}, "o", "i"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "ci"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "vi"), 1);

    EXPECT_EQ(l.gettop(), 0);

    // i/vi won't change, beacuse o is const

    bool failed;
    l.eval<void>("o.i = 2", false, &failed);  // error
    EXPECT_TRUE(failed);
    l.eval<std::tuple<>>("o.i = 2", false, &failed);  // error
    EXPECT_TRUE(failed);
    EXPECT_EQ(l.gettop(), 0);
    EXPECT_EQ(l.get_int({"o", "i"}), 1);

    EXPECT_NE(l.dostring("o.vi = 3"), LUA_OK);  // error
    EXPECT_EQ(l.gettop(), 1);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.vi"), 1);

    EXPECT_EQ(l.gettop(), 0);

    EXPECT_DEATH(l.set({"o", "i"}, 3), ".*Const member.*");
    EXPECT_DEATH(l.set({"o", "vi"}, 3), ".*Const member.*");

    EXPECT_EQ(l.gettop(), 0);
  }

  {
    // set const Obj*

    assert(typeid(const Obj*) != typeid(Obj*));
    EXPECT_NE(typeid(const Obj*).name(), typeid(Obj*).name());

    l.set("o", nullptr);
    EXPECT_EQ(l.get_int({"o", "i"}), 0);

    const Obj  o;
    const Obj* p = &o;
    l.set("o", p);

    EXPECT_EQ(l.eval_int("return o.i"), 1);
    EXPECT_EQ(l.eval_int("return o.ci"), 1);
    EXPECT_EQ(l.eval_int("return o.vi"), 1);

    EXPECT_EQ(l.get_int({"o", "i"}), 1);
    EXPECT_EQ(l.get_int({"o", "ci"}), 1);
    EXPECT_EQ(l.get_int({"o", "vi"}), 1);

    EXPECT_EQ(l.lget<int>({}, "o", "i"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "ci"), 1);
    EXPECT_EQ(l.lget<int>({}, "o", "vi"), 1);

    // i/vi won't change, beacuse o is pointer to const value
    EXPECT_DEATH(l.lset("o", "i", 2), ".*Const member.*");
    EXPECT_NE(l.dostring("o.i = 2"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.get_int({"o", "i"}), 1);

    EXPECT_DEATH(l.lset("o", "vi", 3), ".*Const member.*");
    EXPECT_NE(l.dostring("o.vi = 3"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.vi"), 1);

    EXPECT_EQ(l.gettop(), 0);
  }
}

TEST(register_member, member_functions) {
  luaw l;

  l.register_member("i", &Obj::i);
  l.register_member("plus", &Obj::plus);
  l.register_member("geti", &Obj::geti);

  Obj o;
  l.set("o", &o);

  EXPECT_EQ(l.eval_int("return o:geti()"), 1);
  l.dostring("o:plus()");
  EXPECT_EQ(l.eval_int("return o:geti()"), 2);
  EXPECT_EQ(o.i, 2);
  EXPECT_EQ(l.get_int({"o", "i"}), 2);

  l.register_member<int (Obj::*)() const volatile>("overloaded_f4",
                                                   &Obj::overloaded_f);

  EXPECT_EQ(l.eval_int("return o:overloaded_f4()"), 4002);
  EXPECT_EQ(o.i, 2);

  EXPECT_EQ(l.gettop(), 0);
}

TEST(register_member, member_functions_by_wrong_syntax_call) {
  luaw l;

  l.register_member("i", &Obj::i);
  l.register_member("plus", &Obj::plus);
  l.register_member("plusby", &Obj::plusby);
  l.register_member("reset", &Obj::reset);

  Obj o;
  l.set("o", &o);

  int ret_code = l.dostring("o.plus()");
  EXPECT_NE(ret_code, LUA_OK);
  l.log_error_out();
  EXPECT_EQ(o.i, 1);

  bool failed;
  int  reti = l.eval_int("return o.plus()", -1, false, &failed);
  EXPECT_EQ(reti, -1);
  EXPECT_TRUE(failed);

  ret_code = l.dostring("o:plus()");
  EXPECT_EQ(ret_code, LUA_OK);
  EXPECT_EQ(o.i, 2);

  ret_code = l.dostring("o:reset()");
  EXPECT_EQ(ret_code, LUA_OK);
  EXPECT_EQ(o.i, 0);

  ret_code = l.dostring("o.plusby(3)");
  EXPECT_NE(ret_code, LUA_OK);
  l.log_error_out();
  EXPECT_EQ(o.i, 0);

  EXPECT_EQ(l.eval_int("return o.plusby(o, 5)"), 5);
  EXPECT_EQ(l.eval_int("return o:plusby(5)"), 10);

  EXPECT_EQ(l.gettop(), 0);
}

TEST(register_member, result_status_of_get) {
  luaw l;
  l.set("o", Obj{});

  l.gseek("o");
  EXPECT_FALSE(l.istable());
  EXPECT_TRUE(l.indexable());  // metatable is set when push a custom class
  l.pop();

  {
    bool failed, exists;
    int  ret = l.get_int({"o", "i"}, -1, false, &failed, &exists);
    EXPECT_EQ(ret, -1);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);
  }
  l.register_member("i", &Obj::i);
  {
    bool failed, exists;
    int  ret = l.get_int({"o", "i"}, -1, false, &failed, &exists);
    EXPECT_EQ(ret, 1);
    EXPECT_FALSE(failed);
    EXPECT_TRUE(exists);
  }
  {
    bool failed, exists;
    int  ret = l.get_int({"o", "i", "j"}, -1, false, &failed, &exists);
    EXPECT_EQ(ret, -1);
    EXPECT_TRUE(failed);
    EXPECT_TRUE(exists);
  }

  EXPECT_EQ(l.gettop(), 0);
}

TEST(register_member, member_no_setter) {
  luaw l;
  l.set("o", Obj{});

  bool failed;
  l.eval<void>("o.i = 2", false, &failed);
  EXPECT_TRUE(failed);

  EXPECT_DEATH(l.lset("o", "i", 2), ".*Not found setter.*");
}

TEST(register_member, nonconst_nonvolatile) {
  luaw l;
  l.register_member("plus", &Obj::plus);
  l.register_member("geti", &Obj::geti);
  l.register_member("cv_geti", &Obj::cv_geti);
  {
    const Obj o;
    l.set("o", &o);
    EXPECT_NE(l.dostring("o:plus()"), LUA_OK);
    l.log_error_out();
    bool failed;
    EXPECT_EQ(l.eval<int>("return o:geti()", false, &failed), 1);
    EXPECT_FALSE(failed);
    EXPECT_EQ(l.eval<int>("return o:cv_geti()", false, &failed), 1);
    EXPECT_FALSE(failed);
  }
  {
    volatile Obj o;
    l.set("o", &o);
    EXPECT_NE(l.dostring("o:plus()"), LUA_OK);
    l.log_error_out();
    bool failed;
    EXPECT_EQ(l.eval<int>("return o:geti()", false, &failed), 0);
    EXPECT_TRUE(failed);
    EXPECT_EQ(l.eval<int>("return o:cv_geti()", false, &failed), 1);
    EXPECT_FALSE(failed);
  }
  {
    const volatile Obj o;
    l.set("o", &o);
    EXPECT_NE(l.dostring("o:plus()"), LUA_OK);
    l.log_error_out();
    bool failed;
    EXPECT_EQ(l.eval<int>("return o:geti()", false, &failed), 0);
    EXPECT_TRUE(failed);
    EXPECT_EQ(l.eval<int>("return o:cv_geti()", false, &failed), 1);
    EXPECT_FALSE(failed);
  }
}

TEST(register_member, shared_ptr_member_variable) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);

  {
    auto s = std::make_shared<Obj>();
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const auto s = std::make_shared<Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }

  {
    std::shared_ptr<Obj> s(new Obj);
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const std::shared_ptr<Obj> s(new Obj);
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }

  {
    std::shared_ptr<const Obj> s(new std::add_const_t<Obj>);
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const std::shared_ptr<const Obj> s(new std::add_const_t<Obj>);
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }

  {
    std::shared_ptr<const Obj> s(new Obj);
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const std::shared_ptr<const Obj> s(new Obj);
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
}

TEST(register_member, unique_ptr_member_variable) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);

  {
    auto s = std::make_unique<Obj>();
    // l.set("o", s);  // error
    l.set("o", std::move(s));
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const auto s = std::make_unique<Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }

  {
    std::unique_ptr<Obj> s(new Obj);
    l.set("o", std::move(s));
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const std::unique_ptr<Obj> s(new Obj);
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 2);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }

  {
    std::unique_ptr<const Obj> s(new std::add_const_t<Obj>);
    l.set("o", std::move(s));
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const std::unique_ptr<const Obj> s(new std::add_const_t<Obj>);
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }

  {
    std::unique_ptr<const Obj> s(new Obj);
    l.set("o", std::move(s));
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
  {
    const std::unique_ptr<const Obj> s(new Obj);
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o.ci"), 1);

    EXPECT_EQ(l.eval<int>("o.i = 2; return o.i"), 0);
    EXPECT_EQ(l.eval<int>("o.ci = 2; return o.ci"), 0);
  }
}

TEST(register_member, shared_ptr_member_function) {
  luaw l;
  l.register_member("geti", &Obj::geti);
  l.register_member("plus", &Obj::plus);

  {
    auto s = std::make_shared<Obj>();
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }
  {
    auto s = std::make_shared<Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }

  {
    const auto s = std::make_shared<Obj>();
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }
  {
    const auto s = std::make_shared<Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }

  {
    auto s = std::make_shared<const Obj>();
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=1; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }
  {
    auto s = std::make_shared<const Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=2; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }

  {
    const auto s = std::make_shared<const Obj>();
    l.set("o", s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=3; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }
  {
    const auto s = std::make_shared<const Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=4; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }
}

TEST(register_member, unique_ptr_member_function) {
  luaw l;
  l.register_member("geti", &Obj::geti);
  l.register_member("plus", &Obj::plus);

  {
    auto s = std::make_unique<Obj>();
    l.set("o", std::move(s));
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }
  {
    auto s = std::make_unique<Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }

  {
    const auto s = std::make_unique<Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("o:plus(); return o:geti()"), 2);
  }

  {
    auto s = std::make_unique<const Obj>();
    l.set("o", std::move(s));
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=1; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }
  {
    auto s = std::make_unique<const Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=2; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }

  // 3: const unique_ptr is not movable
  {
    const auto s = std::make_unique<const Obj>();
    l.set("o", &s);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
    EXPECT_EQ(l.eval<int>("x=4; o:plus(); return o:geti()"), 0);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);
  }
}

namespace {
struct ObjDeleter {
  void operator()(Obj* p) const { delete p; }
};
}  // namespace

TEST(register_member, unique_ptr_with_custom_deleter) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member("geti", &Obj::geti);

  {
    std::unique_ptr<Obj, const std::default_delete<Obj>> o(new Obj);
    l.set("o", &o);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);

    l.set("o", std::move(o));
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);

    EXPECT_EQ(l.eval<int>("o.i=2; return o.i"), 2);
  }

  {
    std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
    l.set("o", &o);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);

    l.set("o", std::move(o));
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:geti()"), 1);

    EXPECT_EQ(l.eval<int>("o.i=2; return o.i"), 2);
  }

  {
    std::unique_ptr<Obj[]> o(new Obj[3]);
    l.set("uptr_of_o_array", &o);

    bool failed, exists;
    EXPECT_EQ(l.eval<int>("return uptr_of_o_array.i", false, &failed), 0);
    EXPECT_FALSE(failed);
    EXPECT_EQ(l.get<int>({"uptr_of_o_array", "i"}, false, &failed, &exists), 0);
    EXPECT_FALSE(failed);
    EXPECT_FALSE(exists);

    EXPECT_EQ(l.eval<int>("return uptr_of_o_array:geti()", false, &failed), 0);
    EXPECT_TRUE(failed);

    l.set("uptr_of_o_array", std::move(o));
    EXPECT_EQ(l.eval<int>("return uptr_of_o_array.i"), 0);
    EXPECT_EQ(l.eval<int>("return uptr_of_o_array:geti()"), 0);

    const std::unique_ptr<Obj[]> co(new Obj[3]);
    l.set("uptr_of_o_array", &co);
    EXPECT_EQ(l.eval<int>("return uptr_of_o_array.i"), 0);
    EXPECT_EQ(l.eval<int>("return uptr_of_o_array:geti()"), 0);
  }
}

void seti(Obj* p, int v) { p->i = v; }
int  getci(const Obj* p) { return p->ci; }

TEST(register_member, fake_member_functions) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member<int (Obj ::*)()>("grow",
                                     [](Obj* p) { return p->i += 1000; });

  l.set("o", Obj{});
  EXPECT_EQ(l.eval<int>("return o:grow()"), 1001);
  EXPECT_EQ(l.eval<int>("return o.i"), 1001);

  EXPECT_EQ(l.eval<int>("return o.grow(o)"), 2001);
  EXPECT_EQ(l.eval<int>("return o.i"), 2001);

  l.register_member<void (Obj ::*)(int)>("seti", &seti);
  l.dostring("o:seti(100)");
  EXPECT_EQ(l.eval<int>("return o.i"), 100);

  // fake const member function
  l.register_member<int (Obj::*)() const>("getci", getci);
  l.set("co", std::add_const_t<Obj>{});

  EXPECT_EQ(l.eval_int("return co:getci()"), 1);
  EXPECT_EQ(l.eval_int("return o:getci()"), 1);

  EXPECT_EQ(l.gettop(), 0);
}

int  gi = 100;  // global i
int& getgi(const volatile Obj* o) { return gi; }

TEST(register_member, fake_member_variable_by_cfunction) {
  {
    // register gi as readable/writable member of Obj

    luaw l;
    l.set("o", Obj{});
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw l;
    Obj  o;
    l.set("o", &o);
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw l;
    auto o = std::make_shared<Obj>();
    l.set("o", o);
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw l;
    auto o = std::make_shared<Obj>();
    l.set("o", &o);
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw       l;
    const auto o = std::make_shared<Obj>();
    l.set("o", &o);
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw l;
    auto o = std::make_unique<Obj>();
    l.set("o", &o);
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw l;
    auto o = std::make_unique<Obj>();
    l.set("o", std::move(o));
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }
  {
    luaw       l;
    const auto o = std::make_unique<Obj>();
    l.set("o", &o);
    l.register_member<int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi"), 101);
    EXPECT_EQ(gi, 101);
  }

  {
    // register gi as const member

    luaw l;
    l.set("o", Obj{});

    gi = 100;
    l.register_member<const int Obj::*>("gi", &getgi);

    EXPECT_EQ(l.eval<int>("return o.gi"), gi);
    bool failed;
    EXPECT_EQ(l.eval<int>("o.gi = 101; return o.gi", false, &failed), 0);
    EXPECT_TRUE(failed);
    EXPECT_EQ(gi, 100);
  }
}

TEST(register_member, fake_member_variable_by_lambda) {
  {
    luaw l;
    l.set("o", Obj{});
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }
  {
    luaw l;
    Obj  o;
    l.set("o", &o);
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }
  {
    luaw l;
    l.set("o", std::make_shared<Obj>());
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }
  {
    luaw       l;
    const auto o = std::make_shared<Obj>();
    l.set("o", o);
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }
  {
    luaw       l;
    const auto o = std::make_shared<Obj>();
    l.set("o", &o);
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }
  {
    luaw l;
    l.set("o", std::make_unique<Obj>());
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }
  {
    luaw       l;
    const auto o = std::make_unique<Obj>();
    l.set("o", &o);
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 101; return o.li"), 101);
    EXPECT_EQ(li, 101);
  }

  // by Const obj
  {
    luaw l;
    l.set("o", std::add_const_t<Obj>{});
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), 100);
    EXPECT_NE(l.dostring("o.li = 101"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.li"), 100);
    EXPECT_EQ(li, 100);
  }

  {
    // fake const member

    luaw l;
    l.set("o", Obj{});
    int  li    = 100;
    auto getli = [&](const volatile Obj*) -> int& { return li; };
    l.register_member<const int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    bool failed;
    EXPECT_EQ(l.eval<int>("o.li = 102; return o.li", false, &failed), 0);
    EXPECT_TRUE(failed);
    EXPECT_EQ(li, 100);

    li = 200;
    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("return o.li"), 200);
  }

  {
    // fake const member, use copy captured value in lambda

    luaw l;
    l.set("o", Obj{});
    int  li    = 100;
    auto getli = [=](const volatile Obj*) -> const int& { return li; };
    l.register_member<const int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    bool failed;
    EXPECT_EQ(l.eval<int>("o.li = 103; return o.li", false, &failed), 0);
    EXPECT_TRUE(failed);
    EXPECT_EQ(li, 100);
  }

  {
    // the first argument is ref

    luaw l;
    l.set("o", Obj{});
    int  li    = 100;  // local i
    auto getli = [&](const volatile Obj&) -> int& { return li; };
    l.register_member<int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    EXPECT_EQ(l.eval<int>("o.li = 104; return o.li"), 104);
    EXPECT_EQ(li, 104);
  }

  {
    // fake const member, copy capture,

    luaw l;
    l.set("o", Obj{});
    int  li    = 100;
    auto getli = [=](const volatile Obj&) { return li; };
    l.register_member<const int Obj::*>("li", getli);

    EXPECT_EQ(l.eval<int>("return o.li"), li);
    bool failed;
    EXPECT_EQ(l.eval<int>("o.li = 105; return o.li", false, &failed), 0);
    EXPECT_TRUE(failed);
    EXPECT_EQ(li, 100);

    li = 200;
    EXPECT_EQ(l.eval<int>("return o.li"), 100);
    EXPECT_EQ(li, 200);
  }
}

TEST(register_member, fake_const_member_variable_by_returned_rvalue) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_member("ci", &Obj::ci);

  // define a const member sum as i + ci
  l.register_member<const int Obj::*>(
      "sum", [](const volatile Obj* o) { return o->i + o->ci; });

  Obj o;
  l.set("o", &o);
  EXPECT_EQ(l.eval<int>("return o.sum"), o.i + o.ci);
  EXPECT_EQ(l.eval<int>("o.i = 5; return o.sum"), 5 + o.ci);
  o.i = 6;
  EXPECT_EQ(l.eval<int>("return o.sum"), 6 + o.ci);
}

namespace {
class Obj2 {
public:
  ~Obj2() { delete pi; }
  int* pi = new int(1);
};
}  // namespace

TEST(register_member, fake_member_variable_by_deref_member_pointer) {
  luaw l;
  l.register_member("pi", &Obj2::pi);
  l.register_member<int Obj2::*>(
      "deref_pi", [](const volatile Obj2* o) -> int& { return *o->pi; });

  Obj2 o;
  l.set("o", &o);
  EXPECT_EQ(l.eval<int>("return o.deref_pi"), 1);
  EXPECT_EQ(l.eval<int>("return o.deref_pi"), *o.pi);
  *o.pi = 2;
  EXPECT_EQ(l.eval<int>("return o.deref_pi"), 2);
  EXPECT_EQ(l.eval<int>("o.deref_pi = 3; return o.deref_pi"), 3);
  EXPECT_EQ(*o.pi, 3);
}

TEST(register_member, fake_member_variable_using_addr) {
  luaw l;
  l.register_member<void* const Obj::*>(
      "id", [](const volatile Obj* p) { return (void*)p; });
  {
    Obj o;
    l.set("o", &o);
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)(&o));
  }
  {
    const Obj o;
    l.set("o", &o);
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)(&o));
  }
  {
    auto o = std::make_shared<Obj>();
    l.set("o", o);
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)(o.get()));
    l.set("o2", o);
    EXPECT_TRUE(l.eval<bool>("return o.id == o2.id"));
  }
  {
    auto o = std::make_shared<Obj>();
    l.set("o", &o);
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)(o.get()));
    l.set("o2", &o);
    EXPECT_TRUE(l.eval<bool>("return o.id == o2.id"));
  }
  {
    auto o = std::make_shared<const Obj>();
    l.set("o", o);
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)(o.get()));
  }
  {
    auto o = std::make_shared<const Obj>();
    l.set("o", &o);
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)(o.get()));
  }

  //
  {
    // register a solid obj, id equals to userdata addr
    l.set("o", Obj{});
    EXPECT_EQ(l.eval<void*>("return o.id"), l.get<void*>("o"));
    EXPECT_EQ(l.eval<void*>("return o.id"), (void*)l.get<Obj*>("o"));
    l.set("o2", Obj{});
    EXPECT_TRUE(l.eval<bool>("return o.id ~= o2.id"));
  }
}

double obj_dm_getter(const Obj* o, const char* k) {
  assert(o);
  auto it = o->dm.find(k);
  if (it != o->dm.end()) return it->second;
  return 0.0;
}

void obj_dm_setter(Obj* o, const char* k, double v) {
  assert(o);
  o->dm[k] = v;
}

TEST(register_member, dynamic_member_cfunction) {
  luaw l;
  l.register_dynamic_member(obj_dm_getter, obj_dm_setter);
  EXPECT_EQ(l.gettop(), 0);

  {
    Obj o;
    l.set("o", &o);
    EXPECT_EQ(l.dostring("o.a = 1; o.b = 2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    const Obj o;
    l.set("o", &o);
    EXPECT_NE(l.dostring("o.a = 5; o.b = 6;"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.a;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.b;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  // for smart pointers
  {
    l.set("o", std::make_shared<Obj>());
    EXPECT_EQ(l.dostring("o.a = 1; o.b = 2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  {
    auto o = std::make_shared<Obj>();
    l.set("o", &o);
    EXPECT_EQ(l.dostring("o.a = 1; o.b = 2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  {
    const auto o = std::make_shared<Obj>();
    l.set("o", &o);
    EXPECT_EQ(l.dostring("o.a = 1; o.b = 2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  {
    l.set("o", std::make_unique<Obj>());
    EXPECT_EQ(l.dostring("o.a = 1; o.b = 2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  {
    auto o = std::make_unique<Obj>();
    l.set("o", &o);
    EXPECT_EQ(l.dostring("o.a = 1; o.b = 2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  //
  {
    l.set("o", std::make_shared<const Obj>());
    EXPECT_NE(l.dostring("o.a = 5; o.b = 6;"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.a;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.b;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    l.set("o", std::make_unique<const Obj>());
    EXPECT_NE(l.dostring("o.a = 5; o.b = 6;"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.a;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.b;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
}

TEST(register_member, dynamic_member_lambda) {
  luaw l;
  auto g = [](const Obj* o, const char* k) {
    assert(o);
    auto it = o->dm.find(k);
    if (it != o->dm.end()) return it->second;
    return 0.0;
  };
  auto s = [](Obj* o, const char* k, double v) {
    assert(o);
    o->dm[k] = v;
  };

  l.register_dynamic_member(g, s);
  EXPECT_EQ(l.gettop(), 0);

  {
    l.set("o", Obj{});
    EXPECT_EQ(l.dostring("o.a=1; o.b=2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    Obj o;
    l.set("o", &o);
    EXPECT_EQ(l.dostring("o.a=1; o.b=2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    const Obj o;
    l.set("o", &o);
    EXPECT_NE(l.dostring("o.a=1; o.b=2;"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.a;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.b;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }

  //
  {
    auto o = std::make_shared<Obj>();
    l.set("o", &o);
    EXPECT_EQ(l.dostring("o.a=1; o.b=2;"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return o.a;"), 1);
    EXPECT_EQ(l.eval<int>("return o.b;"), 2);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
  {
    l.set("o", std::make_unique<const Obj>());
    EXPECT_NE(l.dostring("o.a=1; o.b=2;"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.a;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.b;", -1), 0);
    EXPECT_EQ(l.eval_int("return o.c;", -1), 0);
    EXPECT_EQ(l.gettop(), 0);
  }
}

TEST(register_member, dynamic_member_volatile) {
  luaw l;

  auto g = [](const Obj* o, const char* k) {
    assert(o);
    auto it = o->dm.find(k);
    if (it != o->dm.end()) return it->second;
    return 0.0;
  };
  auto s = [](Obj* o, const char* k, double v) {
    assert(o);
    o->dm[k] = v;
  };

  l.register_dynamic_member(g, s);
  EXPECT_EQ(l.gettop(), 0);

  {
    volatile Obj o;
    l.set("o", &o);
    EXPECT_NE(l.dostring("o.a=1; o.b=2;"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return o.a;", -1), -1);
    EXPECT_EQ(l.eval_int("return o.b;", -1), -1);
    EXPECT_EQ(l.eval_int("return o.c;", -1), -1);
    EXPECT_EQ(l.gettop(), 0);
  }
}

namespace {

struct Foo {
  std::unordered_map<std::string, luaw::luavalueref> m;
};
luaw::luavalueref foo_dm_getter(const Foo* o, const std::string& k) {
  auto entry = o->m.find(k);
  if (entry != o->m.end()) { return entry->second; }
  return luaw::luavalueref();
}
void foo_dm_setter(Foo* o, const std::string& k, const luaw::luavalueref& v) {
  o->m[k] = v;
}

}  // namespace

TEST(register_member, luavalueref) {
  luaw l;
  l.register_dynamic_member(foo_dm_getter, foo_dm_setter);
  Foo foo{};
  l.set("foo", &foo);
  l.dostring("foo.a=1 foo.b=true foo.c='str' ");
  EXPECT_EQ(l.eval<int>("return foo.a"), 1);
  EXPECT_EQ(l.eval<bool>("return foo.b"), true);
  EXPECT_EQ(l.eval<std::string>("return foo.c"), "str");

  EXPECT_EQ(l.eval<int>("return foo.d"), 0);

  EXPECT_TRUE(foo.m.count("a"));
  EXPECT_TRUE(foo.m.count("b"));
  EXPECT_TRUE(foo.m.count("c"));
  EXPECT_FALSE(foo.m.count("d"));

  bool failed, exists;
  EXPECT_EQ(l.get<int>({"foo", "d"}, false, &failed, &exists), 0);
  EXPECT_FALSE(failed);
  EXPECT_FALSE(exists);

  l.lset("foo", "a", "x", 1);
  EXPECT_EQ(l.eval<int>("return foo.a.x"), 1);
}

TEST(register_member, get_object_created_by_lua) {
  luaw l;
  l.register_member("i", &Obj::i);
  l.register_ctor<Obj()>("NewObj");
  int retcode = l.dostring("a = NewObj(); a.i = 3;");
  EXPECT_EQ(retcode, LUA_OK);
  Obj a = l.get<Obj>("a");
  EXPECT_EQ(a.i, 3);
  EXPECT_EQ(a.ci, 1);
}
