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

struct Obj {
  // Obj() { puts("Obj()"); }
  // Obj(Obj&&) { puts("Obj(Obj&&)"); }
  // Obj(const Obj&) { puts("Obj(const Obj&)"); }
  // ~Obj() { puts("~Obj()"); }

  int          i  = 1;
  const int    ci = 1;
  volatile int vi = 1;

  int geti() const { return i; }

  int plus() { return ++i; }

  int overloaded_f() { return i + 1000; }
  int overloaded_f() const { return i + 2000; }
  int overloaded_f() volatile { return i + 3000; }
  int overloaded_f() const volatile { return i + 4000; }
  int overloaded_f(int d) { return i + d; }

  void unsupported_fl() & {}
  void unsupported_fr() && {}
};

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
  // l.register_member("unsupported_fl", &Obj::unsupported_fl); // error
  // l.register_member("unsupported_fr", &Obj::unsupported_fr); // error

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

  EXPECT_EQ(l.gettop(), 0);

  // l.register_ctor<Obj>("NewObj"); // error
}
