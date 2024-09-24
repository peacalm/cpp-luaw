// Copyright (c) 2024 Li Shuangquan. All Rights Reserved.
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

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

// Struct definitions for volatile.
// Make the operator= return void to mute GNU compilation warning:
// implicit dereference will not access object of type ‘volatile xxx’ ...

struct A {
  int i;

  A() : i(1) {}
  A(const volatile A& r) : i(r.i) {}
  A(const volatile A&& r) : i(r.i) {}

  void operator=(const volatile A& r) volatile { i = r.i; }
  void operator=(const volatile A&& r) volatile { i = r.i; }
};

struct Ac {
  const int ci;

  Ac() : ci(1) {}
  Ac(const volatile Ac& r) : ci(r.ci) {}
  Ac(const volatile Ac&& r) : ci(r.ci) {}

  void operator=(const volatile Ac& r) volatile  = delete;
  void operator=(const volatile Ac&& r) volatile = delete;
};

struct B {
  int i;
  A   a;

  B() : i(2) {}
  B(const volatile B& r) : i(r.i), a(r.a) {}
  B(const volatile B&& r) : i(r.i), a(std::move(r.a)) {}

  void operator=(const volatile B& r) volatile {
    i = r.i;
    a = r.a;
  }
  void operator=(const volatile B&& r) volatile {
    i = r.i;
    a = r.a;
  }
};

struct Bac {
  Ac ac;

  Bac() {}
  Bac(const volatile Bac& r) : ac(r.ac) {}
  Bac(const volatile Bac&& r) : ac(std::move(r.ac)) {}
};

struct Bcac {
  const Ac cac;

  Bcac() {}
  Bcac(const volatile Bcac& r) : cac(r.cac) {}
  Bcac(const volatile Bcac&& r) : cac(std::move(r.cac)) {}
};

#else

struct A {
  int i = 1;
};

struct Ac {
  const int ci = 11;
};

struct B {
  int i = 2;
  A   a;
};

struct Bac {
  Ac ac;
};

struct Bcac {
  const Ac cac;
};

#endif

TEST(nested_objects, register_nested_members) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("ci", &Ac::ci);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member<const A(B::*)>("a_as_c", &B::a);

  // Error: no operator=
  // l.register_member("ac", &Bac::ac);  // compilation fail

  // OK: set ac as const
  l.register_member<const Ac(Bac::*)>("ac_as_c", &Bac::ac);

  // OK cac already const
  l.register_member("cac", &Bcac::cac);
}

TEST(nested_objects, register_solid_nested_member) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);  // solid member a, which can not be modified

  auto b = std::make_shared<B>();
  l.set("b", b);

  EXPECT_EQ(b->i, 2);
  EXPECT_EQ(l.eval<int>("return b.i"), b->i);
  EXPECT_TRUE(l.dostring("b.i = 5") == LUA_OK);
  EXPECT_TRUE(l.dostring("assert(b.i == 5)") == LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), b->i);
  EXPECT_EQ(b->i, 5);

  // can't modify b's a
  b->a.i           = 1;
  const int oldbai = b->a.i;
  EXPECT_EQ(l.eval<int>("return b.a.i"), oldbai);
  EXPECT_TRUE(l.dostring("b.a.i = 5") == LUA_OK);
  EXPECT_TRUE(l.dostring("assert(b.a.i ~= 5);") == LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), oldbai);
  EXPECT_EQ(b->a.i, oldbai);

  // get a copy of b'a, and can modify the copy's i, but can't modify b.a.i
  EXPECT_EQ(l.dostring("a = b.a"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return a.i"), oldbai);
  EXPECT_EQ(l.dostring("a.i = 123"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return a.i"), 123);       // changed
  EXPECT_EQ(l.eval<int>("return b.a.i"), oldbai);  // no change
}

TEST(nested_objects, register_solid_nested_member_for_const) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);  // solid member a, which can not be modified

  auto b = std::make_shared<const B>();
  l.set("b", b);

  EXPECT_EQ(b->i, 2);
  EXPECT_EQ(l.eval<int>("return b.i"), b->i);
  EXPECT_TRUE(l.dostring("b.i = 5") != LUA_OK);
  l.log_error_out();
  EXPECT_TRUE(l.dostring("assert(b.i ~= 5)") == LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), b->i);
  EXPECT_EQ(b->i, 2);

  // can't modify b's a
  // b->a.i = 1; // Error
  const int oldbai = b->a.i;
  EXPECT_EQ(l.eval<int>("return b.a.i"), oldbai);
  EXPECT_TRUE(l.dostring("b.a.i = 5") != LUA_OK);
  l.log_error_out();
  EXPECT_TRUE(l.dostring("assert(b.a.i ~= 5);") == LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), oldbai);
  EXPECT_EQ(b->a.i, oldbai);

  // get a copy of b'a, can't modify the copy's i too!
  EXPECT_EQ(l.dostring("a = b.a"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return a.i"), oldbai);
  EXPECT_TRUE(l.dostring("a.i = 123") != LUA_OK);
  l.log_error_out();
  EXPECT_NE(l.eval<int>("return a.i"), 123);
  EXPECT_EQ(l.eval<int>("return a.i"), oldbai);
  EXPECT_EQ(l.eval<int>("return b.a.i"), oldbai);
}

TEST(nested_objects, register_member_address) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  // get a's non-const ptr.
  // The first arg could be "const B*" or "auto*", both ok.
  l.register_member<A* const B::*>(
      "aptr", [](const B* p) { return &(const_cast<B*>(p)->a); });
  l.register_member<A* const B::*>(
      "aptr2", [](auto* p) { return &(const_cast<B*>(p)->a); });

  EXPECT_TRUE(l.dostring("assert(b.aptr == b.aptr2)") == LUA_OK);
  EXPECT_EQ(l.eval<void*>("return b.aptr"), (void*)(&b->a));
  EXPECT_EQ(l.eval<void*>("return b.aptr2"), (void*)(&b->a));
  {
    auto t = l.eval<std::tuple<void*, void*>>("return b.aptr, b.aptr2");
    EXPECT_EQ(std::get<0>(t), std::get<1>(t));
  }

#if 0
  {
    // maybe compile error after cpp17
    auto vb = std::make_shared<volatile B>();

    l.set("vb", vb);
    EXPECT_TRUE(l.dostring("assert(vb.aptr == vb.aptr2)") == LUA_OK);
    bool supportv = PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT;
    EXPECT_EQ((l.eval<void*>("return vb.aptr") == (void*)(&vb->a)), supportv);
    EXPECT_EQ((l.eval<void*>("return vb.aptr2") == (void*)(&vb->a)), supportv);
  }
#endif

  // get a's const ptr
  l.register_member<const A* const B::*>("acptr",
                                         [](const B* p) { return &(p->a); });
  l.register_member<const A* const B::*>("acptr2",
                                         [](auto* p) { return &(p->a); });

  EXPECT_TRUE(l.dostring("assert(b.acptr == b.acptr2)") == LUA_OK);
  EXPECT_EQ(l.eval<void*>("return b.acptr"), (void*)(&b->a));
  EXPECT_EQ(l.eval<void*>("return b.acptr2"), (void*)(&b->a));
  {
    auto t = l.eval<std::tuple<void*, void*>>("return b.acptr, b.acptr2");
    EXPECT_EQ(std::get<0>(t), std::get<1>(t));
  }

  // check metatable of ptr
  {
    EXPECT_EQ(l.gettop(), 0);

    EXPECT_EQ(l.dostring("aptr = b.aptr"), LUA_OK);
    l.gseek("aptr");
    std::string mtaptr = l.get_metatable_name(-1);
    l.pop();

    EXPECT_EQ(l.dostring("acptr = b.acptr"), LUA_OK);
    l.gseek("acptr");
    std::string mtacptr = l.get_metatable_name(-1);
    l.pop();

    watch(mtaptr, mtacptr);
    EXPECT_NE(mtaptr, mtacptr);

    EXPECT_EQ(l.gettop(), 0);

    {
      l.eval<void>("print('b.aptr  = ', b.aptr, b.aptr2)");
      l.eval<void>("print('b.acptr = ', b.acptr, b.acptr2)");

      // metatable of aptr now equals to acptr's
      l.eval<void>("print('aptr  = ', aptr)");
      l.eval<void>("print('acptr = ', acptr)");
    }

    puts("");

    {
      // modify order
      l.eval<void>("print('b.acptr = ', b.acptr, b.acptr2)");
      l.eval<void>("print('b.aptr  = ', b.aptr, b.aptr2)");

      // metatable of acptr now equals to aptr's
      l.eval<void>("print('aptr  = ', aptr)");
      l.eval<void>("print('acptr = ', acptr)");
    }
  }

  // change member a
  {
    EXPECT_EQ(b->a.i, 1);
    EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
    b->a.i = 2;
    EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
    A* p = l.eval<A*>("return b.aptr");
    p->i = 3;
    EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
    EXPECT_EQ(b->a.i, 3);

    // modify b.a.i in Lua by aptr
    EXPECT_EQ(l.eval<int>("return b.aptr.i"), 3);
    EXPECT_EQ(l.dostring("b.aptr.i = 4"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return b.aptr.i"), 4);
    EXPECT_EQ(l.eval<int>("return b.acptr.i"), 4);
    EXPECT_EQ(b->a.i, 4);
    EXPECT_EQ(p->i, 4);

    // can't modify by acptr
    EXPECT_NE(l.dostring("b.acptr.i = 5"), LUA_OK);
    l.log_error_out();
  }
}

#if !PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

TEST(nested_objects, register_member_by_fake_shared_ptr) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);

  // Compilation fail if support volatile B.
  l.register_member<std::shared_ptr<A> B::*>("aref", [](const B* p) {
    // a fake shared_ptr, whose deleter does nothing
    return std::shared_ptr<A>(&const_cast<B*>(p)->a, [](...) {});
  });

  auto b = std::make_shared<B>();
  l.set("b", b);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 1);

  // can modify b.a.i by b's aref
  EXPECT_EQ(l.dostring("b.aref.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 2);

  {
    // make a copy of b's a by aref
    EXPECT_EQ(l.dostring("a = b.aref"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return a.i"), 2);
    EXPECT_EQ(l.eval<int>("return b.a.i"), 2);

    // also can modify b.a.i by the copy
    EXPECT_EQ(l.dostring("a.i = 3"), LUA_OK);
    EXPECT_EQ(l.eval<int>("return a.i"), 3);
    EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
    EXPECT_EQ(l.eval<int>("return b.aref.i"), 3);
  }
}

#endif

struct Bval {
  int i;
  A   a;

  Bval() : i(1) {}
  Bval(const volatile Bval& r) : i(r.i) { a.i = r.a.i; }
  Bval(const volatile Bval&& r) : i(r.i) { a.i = r.a.i; }

  void operator=(const volatile Bval& r) volatile {
    i   = r.i;
    a.i = r.a.i;
  }
  void operator=(const volatile Bval&& r) volatile {
    i   = r.i;
    a.i = r.a.i;
  }
};

TEST(nested_objects, volatile_outer) {
  peacalm::luaw l;

  l.register_member("i", &A::i);
  l.register_member("i", &Bval::i);
  l.register_member("a", &Bval::a);

  l.register_member<A* const Bval::*>(
      "aptr", [](auto* p) { return &(const_cast<Bval*>(p)->a); });
  l.register_member<const A* const Bval::*>(
      "acptr", [](auto* p) { return &(const_cast<Bval*>(p)->a); });

  // Error: can not construct `volatile std::shared_ptr<A>`
  // l.register_member<std::shared_ptr<A> Bval::*>("aref", [](auto* p) {
  //   return std::shared_ptr<A>(&const_cast<Bval*>(p)->a, [](...) {});
  // });

  volatile Bval b;
  b.a.i = 1;
  b.i   = 1;
  l.set("b", b);

  bool failed;
  int  bi = l.eval_int("return b.i", 0, false, &failed);
  EXPECT_FALSE(failed);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
  EXPECT_EQ(bi, 1);
#else
  EXPECT_EQ(bi, 0);
#endif

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
  EXPECT_EQ(l.eval<int>("return b.i"), 1);
  EXPECT_EQ(l.dostring("b.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), 2);

  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);

  // can't modify b.a.i by b.a.i
  EXPECT_EQ(l.dostring("b.a.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);

  // modify b.a.i by aptr
  EXPECT_EQ(l.dostring("b.aptr.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 2);

#else
  EXPECT_EQ(l.eval<int>("return b.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 0);
  // EXPECT_EQ(l.eval<int>("return b.aref.i"), 0);

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_NE(l.dostring("b.i = 2"), LUA_OK);
  l.log_error_out();
  EXPECT_NE(l.dostring("b.aptr.i = 2"), LUA_OK);
  l.log_error_out();

  EXPECT_EQ(l.gettop(), 0);
#endif
}

struct C {
  volatile Bval b;
};

TEST(nested_objects, member_volatile) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("a", &Bval::a);
  l.register_member("i", &Bval::i);
  l.register_member("b", &C::b);

  l.register_member<A* const Bval::*>(
      "aptr", [](auto* p) { return &(const_cast<Bval*>(p)->a); });

  l.register_member<volatile Bval* const C::*>(
      "bptr", [](auto* p) { return &(const_cast<C*>(p)->b); });

  auto c = std::make_shared<C>();
  l.set("c", c);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

  EXPECT_EQ(l.eval<int>("return c.b.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 1);

  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 1);

  EXPECT_EQ(l.dostring("c.bptr.i = 3;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.i"), 3);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 3);

  EXPECT_EQ(l.dostring("c.bptr.aptr.i = 4;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 4);

#else
  EXPECT_EQ(l.eval<int>("return c.b.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 0);

  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 0);
#endif

  EXPECT_EQ(l.gettop(), 0);
}

}  // namespace
