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

#if 1
namespace {

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

TEST(nested_objects, register_nested_members) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("ci", &Ac::ci);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member<const A(B::*)>("a_as_c", &B::a);

  // l.register_member("ac", &Bac::ac); // Error: no operator=
  l.register_member<const Ac(Bac::*)>("ac_as_c", &Bac::ac);  // OK: set ac const

  l.register_member("cac", &Bcac::cac);  // OK cac already const
}

TEST(nested_objects, register_solid_nested_member) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);  // solid member a, which can not be modified

  auto b = std::make_shared<B>();
  l.set("b", b);

  EXPECT_EQ(l.eval<int>("return b.i"), b->i);
  EXPECT_TRUE(l.dostring("b.i = 5") == LUA_OK);
  EXPECT_TRUE(l.dostring("assert(b.i == 5)") == LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), b->i);
  EXPECT_EQ(b->i, 5);

  // can't modify b's a
  b->a.i           = 1;
  const int oldbai = b->a.i;
  EXPECT_EQ(l.eval<int>("return b.a.i"), b->a.i);
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

    // can't modify by acptr
    EXPECT_NE(l.dostring("b.acptr.i = 5"), LUA_OK);
    l.log_error_out();
  }
}

TEST(nested_objects, register_member_by_fake_shared_ptr) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);

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

struct BVolatileCompleted {
  BVolatileCompleted() {}

  // copy ctor
  BVolatileCompleted(const volatile BVolatileCompleted& r) {
    i   = r.i;
    a.i = r.a.i;
  }

  // move ctor
  BVolatileCompleted(volatile BVolatileCompleted&& r) {
    i   = r.i;
    a.i = r.a.i;
  }

  // copy by const rvalue ref ???
  BVolatileCompleted(const volatile BVolatileCompleted&& r) {
    i   = r.i;
    a.i = r.a.i;
  }

  ~BVolatileCompleted() {}

  auto& operator=(const volatile BVolatileCompleted& r) volatile {
    i   = r.i;
    a.i = r.a.i;
    return *this;
  }

  auto& operator=(volatile BVolatileCompleted&& r) volatile {
    i   = r.i;
    a.i = r.a.i;
    return *this;
  }

  auto& operator=(const volatile BVolatileCompleted&& r) volatile {
    i   = r.i;
    a.i = r.a.i;
    return *this;
  }

  int i = 2;
  A   a;
};

static void test_BVolatileCompleted() {
#define TEST_VOLATILE_CTORS(ATYPE, OTYPE, ASSIGN) \
  {                                               \
    ATYPE a;                                      \
    OTYPE b = a;                                  \
    OTYPE c(a);                                   \
    OTYPE d = std::move(a);                       \
    OTYPE e(std::move(a));                        \
    ASSIGN                                        \
  }

#define TEST_ASSIGN \
  b = a;            \
  c = std::move(a);

#define NO_TEST_ASSIGN

  using B   = BVolatileCompleted;
  using CB  = const B;
  using VB  = volatile B;
  using CVB = const volatile B;

  TEST_VOLATILE_CTORS(B, B, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(B, CB, NO_TEST_ASSIGN);
  TEST_VOLATILE_CTORS(B, VB, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(B, CVB, NO_TEST_ASSIGN);

  TEST_VOLATILE_CTORS(CB, B, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(CB, CB, NO_TEST_ASSIGN);
  TEST_VOLATILE_CTORS(CB, VB, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(CB, CVB, NO_TEST_ASSIGN);

  TEST_VOLATILE_CTORS(VB, B, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(VB, CB, NO_TEST_ASSIGN);
  TEST_VOLATILE_CTORS(VB, VB, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(VB, CVB, NO_TEST_ASSIGN);

  TEST_VOLATILE_CTORS(CVB, B, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(CVB, CB, NO_TEST_ASSIGN);
  TEST_VOLATILE_CTORS(CVB, VB, TEST_ASSIGN);
  TEST_VOLATILE_CTORS(CVB, CVB, NO_TEST_ASSIGN);

#undef NO_TEST_ASSIGN
#undef TEST_ASSIGN
#undef TEST_VOLATILE_CTORS
}

TEST(nested_objects, volatile_outer) {
  peacalm::luaw l;

  using B = BVolatileCompleted;

  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);

  l.register_member<A* const B::*>(
      "aptr", [](auto* p) { return &(const_cast<B*>(p)->a); });
  l.register_member<const A* const B::*>(
      "acptr", [](auto* p) { return &(const_cast<B*>(p)->a); });

  l.register_member<std::shared_ptr<A> B::*>("aref", [](auto* p) {
    return std::shared_ptr<A>(&const_cast<B*>(p)->a, [](...) {});
  });

  volatile B b;
  b.a.i = 1;
  b.i   = 2;
  l.set("b", b);

  bool failed;
  int  bi = l.eval_int("return b.i", 0, false, &failed);
  EXPECT_FALSE(failed);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
  EXPECT_EQ(bi, 2);
#else
  EXPECT_EQ(bi, 0);
#endif

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT
  EXPECT_EQ(l.dostring("b.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.i"), 2);

  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 1);

  EXPECT_EQ(l.dostring("b.aptr.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 2);

  EXPECT_EQ(l.dostring("b.aref.i = 3"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 3);
#else
  EXPECT_EQ(l.eval<int>("return b.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 0);

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_NE(l.dostring("b.i = 2"), LUA_OK);
  l.log_error_out();
  EXPECT_NE(l.dostring("b.aptr.i = 2"), LUA_OK);
  l.log_error_out();
  EXPECT_NE(l.dostring("b.aref.i = 3"), LUA_OK);
  l.log_error_out();

  EXPECT_EQ(l.gettop(), 0);
#endif
}

struct C {
  volatile BVolatileCompleted b;
};

TEST(nested_objects, member_volatile) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("a", &BVolatileCompleted::a);
  l.register_member("i", &BVolatileCompleted::i);
  l.register_member("b", &C::b);

  l.register_member<A* const BVolatileCompleted::*>(
      "aptr", [](auto* p) { return &(const_cast<BVolatileCompleted*>(p)->a); });

  l.register_member<volatile BVolatileCompleted* const C::*>(
      "bptr", [](auto* p) { return &(const_cast<C*>(p)->b); });

  auto c = std::make_shared<C>();
  l.set("c", c);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

  EXPECT_EQ(l.eval<int>("return c.b.i"), 2);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 2);

  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 1);

  EXPECT_EQ(l.dostring("c.bptr.i = 3;"), LUA_OK);
  EXPECT_EQ(l.dostring("c.bptr.aptr.i = 4;"), LUA_OK);

  EXPECT_EQ(l.eval<int>("return c.b.i"), 3);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 3);

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

  auto the_PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT =
      PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT;
  watch(the_PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT);
}

}  // namespace
#endif
