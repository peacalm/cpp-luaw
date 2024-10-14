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

struct Bap {
  int                i;
  A*                 ap;
  std::shared_ptr<A> sa;
};

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

  // solid member a, which can not modify it's member by b.a.i = xxx;
  l.register_member("a", &B::a);

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

  // can modify the whole member a
  EXPECT_EQ(l.dostring("b.a = a"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 123);  // changed!
}

TEST(nested_objects, register_solid_nested_member_for_const) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);

  // solid member a, which can not modify it's member by b.a.i = xxx;
  l.register_member("a", &B::a);

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

  // can't modify the whole member a
  EXPECT_NE(l.dostring("b.a = a"), LUA_OK);
  l.log_error_out();
}

TEST(nested_objects, register_member_ptr_by_shared_ptr_nonconst_obj) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ptr("aptr", &B::a);
  l.register_member_cptr("acptr", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  // check metatable of ptr and cptr
  {
    EXPECT_EQ(l.gettop(), 0);
    auto _g = l.make_guarder();

    EXPECT_EQ(l.dostring("aptr = b.aptr"), LUA_OK);
    l.gseek("aptr");
    std::string mtaptr = l.get_metatable_name(-1);

    EXPECT_EQ(l.dostring("acptr = b.acptr"), LUA_OK);
    l.gseek("acptr");
    std::string mtacptr = l.get_metatable_name(-1);

    watch(mtaptr, mtacptr);
    EXPECT_NE(mtaptr, mtacptr);

    l.gseek("aptr");
    std::string mtaptr_again = l.get_metatable_name(-1);

    watch(mtaptr_again, mtacptr);
    EXPECT_NE(mtaptr_again, mtaptr);  // metatable has changed
    EXPECT_EQ(mtaptr_again, mtacptr);
  }
  EXPECT_EQ(l.gettop(), 0);
  // check value of ptr and cptr
  {
    EXPECT_EQ(l.dostring("assert(b.aptr == b.acptr)"), LUA_OK);
    void* p  = l.eval<void*>("return b.aptr");
    void* cp = l.eval<void*>("return b.acptr");
    EXPECT_EQ(p, cp);
  }

  EXPECT_EQ(b->a.i, 1);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);

  b->a.i = 2;
  EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 2);

  A* p = l.eval<A*>("return b.aptr");
  p->i = 3;
  EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 3);
  EXPECT_EQ(b->a.i, 3);

  // modify b.a.i in Lua by aptr
  EXPECT_EQ(l.dostring("b.aptr.i = 4"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 4);
  EXPECT_EQ(b->a.i, 4);
  EXPECT_EQ(p->i, 4);

  // can't modify by acptr
  EXPECT_NE(l.dostring("b.acptr.i = 5"), LUA_OK);
  l.log_error_out();
}

TEST(nested_objects, register_member_ptr_by_shared_ptr_const_obj) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ptr("aptr", &B::a);  // equivalent to acptr
  l.register_member_cptr("acptr", &B::a);

  auto b = std::make_shared<const B>();
  l.set("b", b);

  // check metatable of ptr and cptr
  {
    EXPECT_EQ(l.gettop(), 0);
    auto _g = l.make_guarder();

    EXPECT_EQ(l.dostring("aptr = b.aptr"), LUA_OK);
    l.gseek("aptr");
    std::string mtaptr = l.get_metatable_name(-1);

    EXPECT_EQ(l.dostring("acptr = b.acptr"), LUA_OK);
    l.gseek("acptr");
    std::string mtacptr = l.get_metatable_name(-1);

    watch(mtaptr, mtacptr);
    EXPECT_EQ(mtaptr, mtacptr);  // Equal
  }
  EXPECT_EQ(l.gettop(), 0);
  // check value of ptr and cptr
  {
    EXPECT_EQ(l.dostring("assert(b.aptr == b.acptr)"), LUA_OK);
    void* p  = l.eval<void*>("return b.aptr");
    void* cp = l.eval<void*>("return b.acptr");
    EXPECT_EQ(p, cp);
  }

  EXPECT_EQ(b->a.i, 1);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);

  // can't modify b.a.i in Lua by aptr
  EXPECT_NE(l.dostring("b.aptr.i = 2"), LUA_OK);
  l.log_error_out();
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);
  EXPECT_EQ(b->a.i, 1);

  // can't modify by acptr too
  EXPECT_NE(l.dostring("b.acptr.i = 3"), LUA_OK);
  l.log_error_out();
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);

  // forcely get non const A* from const A*
  A* p = l.eval<A*>("return b.aptr");
  p->i = 4;  // can modify by non const ptr
  EXPECT_EQ(l.eval<int>("return b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 4);
  EXPECT_EQ(b->a.i, 4);
}

TEST(nested_objects, register_member_ptr_by_nonconst_obj) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ptr("aptr", &B::a);
  l.register_member_cptr("acptr", &B::a);

  B b;
  l.set("b", b);

  // check metatable of ptr and cptr
  {
    EXPECT_EQ(l.gettop(), 0);
    auto _g = l.make_guarder();

    EXPECT_EQ(l.dostring("aptr = b.aptr"), LUA_OK);
    l.gseek("aptr");
    std::string mtaptr = l.get_metatable_name(-1);

    EXPECT_EQ(l.dostring("acptr = b.acptr"), LUA_OK);
    l.gseek("acptr");
    std::string mtacptr = l.get_metatable_name(-1);

    watch(mtaptr, mtacptr);
    EXPECT_NE(mtaptr, mtacptr);

    l.gseek("aptr");
    std::string mtaptr_again = l.get_metatable_name(-1);

    watch(mtaptr_again, mtacptr);
    EXPECT_NE(mtaptr_again, mtaptr);  // metatable has changed
    EXPECT_EQ(mtaptr_again, mtacptr);
  }
  EXPECT_EQ(l.gettop(), 0);
  // check value of ptr and cptr
  {
    EXPECT_EQ(l.dostring("assert(b.aptr == b.acptr)"), LUA_OK);
    void* p  = l.eval<void*>("return b.aptr");
    void* cp = l.eval<void*>("return b.acptr");
    EXPECT_EQ(p, cp);
  }

  EXPECT_EQ(b.a.i, 1);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);

  A* p = l.eval<A*>("return b.aptr");
  p->i = 3;
  EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 3);
  EXPECT_EQ(p->i, 3);

  // modify b.a.i in Lua by aptr
  EXPECT_EQ(l.dostring("b.aptr.i = 4"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 4);
  EXPECT_EQ(p->i, 4);

  // can't modify by acptr
  EXPECT_NE(l.dostring("b.acptr.i = 5"), LUA_OK);
  l.log_error_out();
}

TEST(nested_objects, register_member_ptr_by_const_obj) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ptr("aptr", &B::a);
  l.register_member_cptr("acptr", &B::a);

  const B b;
  l.set("b", b);

  // check metatable of ptr and cptr
  {
    EXPECT_EQ(l.gettop(), 0);
    auto _g = l.make_guarder();

    EXPECT_EQ(l.dostring("aptr = b.aptr"), LUA_OK);
    l.gseek("aptr");
    std::string mtaptr = l.get_metatable_name(-1);

    EXPECT_EQ(l.dostring("acptr = b.acptr"), LUA_OK);
    l.gseek("acptr");
    std::string mtacptr = l.get_metatable_name(-1);

    watch(mtaptr, mtacptr);
    EXPECT_EQ(mtaptr, mtacptr);
  }
  EXPECT_EQ(l.gettop(), 0);
  // check value of ptr and cptr
  {
    EXPECT_EQ(l.dostring("assert(b.aptr == b.acptr)"), LUA_OK);
    void* p  = l.eval<void*>("return b.aptr");
    void* cp = l.eval<void*>("return b.acptr");
    EXPECT_EQ(p, cp);
  }

  EXPECT_EQ(b.a.i, 1);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);

  A* p = l.eval<A*>("return b.aptr");
  p->i = 3;
  EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 3);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 3);
  EXPECT_EQ(p->i, 3);

  // modify b.a.i in Lua by aptr
  EXPECT_NE(l.dostring("b.aptr.i = 4"), LUA_OK);
  l.log_error_out();

  // can't modify by acptr
  EXPECT_NE(l.dostring("b.acptr.i = 5"), LUA_OK);
  l.log_error_out();
}

TEST(nested_objects, register_member_ptr_invalid_ways) {
  peacalm::luaw l;

  // Class must be decayed
  using CB = const B;
  l.register_member_ptr("aptr", &CB::a);  // ok. auto decay
  // l.register_member_ptr<CB>("aptr", &CB::a);       // error
  // l.register_member_ptr<CB, A>("aptr", &CB::a);    // error
  // l.register_member_ptr<const B>("aptr", &CB::a);  // error
  // l.register_member_ptr<CB>("aptr", &B::a);        // error

  // ok! register ptr for int member
  l.register_member_ptr("iptr", &Bap::i);
  l.register_member_cptr("icptr", &Bap::i);

  // No need to register pointer for pointer members
  // l.register_member_ptr("apptr", &Bap::ap);    // error
  // l.register_member_cptr("apcptr", &Bap::ap);  // error

  // No need to register pointer for smart ptr member
  // l.register_member_ptr("saptr", &Bap::sa);    // error
  // l.register_member_cptr("sacptr", &Bap::sa);  // error
}

TEST(nested_objects, register_member_ref_invalid_ways) {
  peacalm::luaw l;

  // Class must be decayed
  using CB = const B;
  l.register_member_ref("aref", &CB::a);  // ok. auto decay
  // l.register_member_ref<CB>("aref", &CB::a);       // error
  // l.register_member_ref<CB, A>("aref", &CB::a);    // error
  // l.register_member_ref<const B>("aref", &CB::a);  // error
  // l.register_member_ref<CB>("aref", &B::a);        // error

  // ok! register reference for int member
  l.register_member_ref("iref", &Bap::i);
  l.register_member_cref("icref", &Bap::i);

  // No need to register reference for pointer members
  // l.register_member_ref("apref", &Bap::ap);    // error
  // l.register_member_cref("apcref", &Bap::ap);  // error

  // No need to register reference for smart ptr member
  // l.register_member_ref("saptr", &Bap::sa);    // error
  // l.register_member_cref("sacptr", &Bap::sa);  // error
}

TEST(nested_objects, register_member_ref_by_shared_ptr_nonconst_obj) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ref("aref", &B::a);
  l.register_member_cref("acref", &B::a);

  auto b = std::make_shared<B>();
  l.set("b", b);

  // check metatable of ref and cref
  {
    EXPECT_EQ(l.gettop(), 0);
    auto _g = l.make_guarder();

    EXPECT_EQ(l.dostring("aref = b.aref"), LUA_OK);
    l.gseek("aref");
    std::string mtaref = l.get_metatable_name(-1);

    EXPECT_EQ(l.dostring("acref = b.acref"), LUA_OK);
    l.gseek("acref");
    std::string mtacref = l.get_metatable_name(-1);

    watch(mtaref, mtacref);
    EXPECT_NE(mtaref, mtacref);

    l.gseek("aref");
    std::string mtaref_again = l.get_metatable_name(-1);

    watch(mtaref_again, mtaref);
    EXPECT_EQ(mtaref_again, mtaref);  // metatable does not change
    EXPECT_NE(mtaref_again, mtacref);
  }
  EXPECT_EQ(l.gettop(), 0);

  // check value of ref and cref
  {
    EXPECT_EQ(l.dostring("assert(b.aref ~= b.acref)"), LUA_OK);

    bool  failed1 = true, failed2 = true;
    void* p  = l.eval<void*>("return b.aref", false, &failed1);
    void* cp = l.eval<void*>("return b.acref", false, &failed2);

    EXPECT_NE(p, cp);
    EXPECT_FALSE(failed1);
    EXPECT_FALSE(failed2);

    // each time call b.aref, it will build a new ref
    void* p2 = l.eval<void*>("aref = b.aref; return aref");
    void* p3 = l.eval<void*>("return aref");
    // EXPECT_NE(p, p2);  // Can be not equal, but do not make assurance
    EXPECT_EQ(p2, p3);
  }

  EXPECT_EQ(b->a.i, 1);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 1);

  b->a.i = 2;
  EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 2);

  // modify b.a.i in Lua by aptr
  EXPECT_EQ(l.dostring("b.aref.i = 4"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 4);
  EXPECT_EQ(b->a.i, 4);

  // can't modify by acptr
  EXPECT_NE(l.dostring("b.acref.i = 5"), LUA_OK);
  l.log_error_out();
}

TEST(nested_objects, register_member_ref_by_shared_ptr_const_obj) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ref("aref", &B::a);
  l.register_member_cref("acref", &B::a);

  auto b = std::make_shared<const B>();
  l.set("b", b);

  // check metatable of ref and cref
  {
    EXPECT_EQ(l.gettop(), 0);
    auto _g = l.make_guarder();

    EXPECT_EQ(l.dostring("aref = b.aref"), LUA_OK);
    l.gseek("aref");
    std::string mtaref = l.get_metatable_name(-1);

    EXPECT_EQ(l.dostring("acref = b.acref"), LUA_OK);
    l.gseek("acref");
    std::string mtacref = l.get_metatable_name(-1);

    watch(mtaref, mtacref);
    EXPECT_EQ(mtaref, mtacref);
  }
  EXPECT_EQ(l.gettop(), 0);

  // check value of ref and cref
  {
    EXPECT_EQ(l.dostring("assert(b.aref ~= b.acref)"), LUA_OK);

    bool  failed1 = true, failed2 = true;
    void* p  = l.eval<void*>("return b.aref", false, &failed1);
    void* cp = l.eval<void*>("return b.acref", false, &failed2);

    EXPECT_NE(p, cp);
    EXPECT_FALSE(failed1);
    EXPECT_FALSE(failed2);

    // each time call b.aref, it will build a new ref
    void* p2 = l.eval<void*>("aref = b.aref; return aref");
    void* p3 = l.eval<void*>("return aref");
    // EXPECT_NE(p, p2);  // Can be not equal, but do not make assurance
    EXPECT_EQ(p2, p3);
  }

  EXPECT_EQ(b->a.i, 1);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 1);

  // can't modify b.a.i in Lua by aptr
  EXPECT_NE(l.dostring("b.aref.i = 4"), LUA_OK);
  l.log_error_out();

  // can't modify by acptr
  EXPECT_NE(l.dostring("b.acref.i = 5"), LUA_OK);
  l.log_error_out();
}

TEST(nested_objects, register_member_address_manually) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ptr("aptr", &B::a);
  l.register_member_cptr("acptr", &B::a);

  l.register_member<void* const B::*>("aid",
                                      [](auto* p) { return (void*)(&(p->a)); });

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

  l.register_member<const volatile void* const B::*>(
      "aid2", [](const volatile B* p) { return &(p->a); });

#else

  l.register_member<const void* const B::*>("aid2",
                                            [](const B* p) { return &(p->a); });

#endif

  auto b = std::make_shared<B>();
  l.set("b", b);

  // l.dostring("print('b.aid   =', b.aid)");  // no metatable
  // l.dostring("print('b.aid2  =', b.aid2)"); // no metatable
  // l.dostring("print('b.aptr  =', b.aptr)");
  // l.dostring("print('b.acptr =', b.acptr)");

  EXPECT_EQ(l.eval<void*>("return b.aid"), (void*)(&(b->a)));

  EXPECT_EQ(l.eval<void*>("return b.aid"), l.eval<void*>("return b.aid2"));
  EXPECT_EQ(l.eval<void*>("return b.aid"), l.eval<void*>("return b.aptr"));
  EXPECT_EQ(l.eval<void*>("return b.aid"), l.eval<void*>("return b.acptr"));

  EXPECT_EQ(l.dostring("assert(b.aid == b.aid2)"), LUA_OK);
  EXPECT_EQ(l.dostring("assert(b.aid == b.aptr)"), LUA_OK);
  EXPECT_EQ(l.dostring("assert(b.aid == b.acptr)"), LUA_OK);
}

TEST(nested_objects, register_member_always_non_const_ptr) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("i", &B::i);
  l.register_member("a", &B::a);
  l.register_member_ptr("aptr", &B::a);

  // register a's always non-const ptr.
  // The arg of lambda could be "const volatile B* p" or "auto* p", both ok.
  // Cast p to no cv- in function body.
  l.register_member<A* const B::*>(
      "aptr2", [](auto* p) { return &(const_cast<B*>(p)->a); });

  auto b = std::make_shared<const B>();
  l.set("b", b);

  EXPECT_TRUE(l.dostring("assert(b.aptr == b.aptr2)") == LUA_OK);
  EXPECT_EQ(l.eval<void*>("return b.aptr"), (void*)(&b->a));
  EXPECT_EQ(l.eval<void*>("return b.aptr2"), (void*)(&b->a));
  {
    auto t = l.eval<std::tuple<void*, void*>>("return b.aptr, b.aptr2");
    EXPECT_EQ(std::get<0>(t), std::get<1>(t));
  }

  // check metatable of ptr
  {
    EXPECT_EQ(l.gettop(), 0);

    EXPECT_EQ(l.dostring("aptr = b.aptr"), LUA_OK);
    l.gseek("aptr");
    std::string mtaptr = l.get_metatable_name(-1);
    l.pop();

    EXPECT_EQ(l.dostring("aptr2 = b.aptr2"), LUA_OK);
    l.gseek("aptr2");
    std::string mtaptr2 = l.get_metatable_name(-1);
    l.pop();

    watch(mtaptr, mtaptr2);
    EXPECT_NE(mtaptr, mtaptr2);

    EXPECT_EQ(l.gettop(), 0);
  }

  // change member a
  {
    EXPECT_EQ(b->a.i, 1);
    EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
    EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
    EXPECT_EQ(l.eval<int>("return b.aptr2.i"), 1);

    // can't modify b.a.i by aptr, because b is const
    EXPECT_NE(l.dostring("b.aptr.i = 2"), LUA_OK);
    l.log_error_out();

    // can modify b.a.i by aptr2, it is always non-const
    EXPECT_EQ(l.dostring("b.aptr2.i = 3"), LUA_OK);
    EXPECT_EQ(b->a.i, 3);
    EXPECT_EQ(l.eval<int>("return b.a.i"), 3);
    EXPECT_EQ(l.eval<int>("return b.aptr.i"), 3);
    EXPECT_EQ(l.eval<int>("return b.aptr2.i"), 3);

    // get aptr into C++ to non-const,
    A* p = l.eval<A*>("return b.aptr");
    p->i = 4;
    EXPECT_EQ(l.eval<int>("return b.a.i"), 4);
    EXPECT_EQ(l.eval<int>("return b.aptr.i"), 4);
    EXPECT_EQ(l.eval<int>("return b.aptr2.i"), 4);
    EXPECT_EQ(b->a.i, 4);
    EXPECT_EQ(p->i, 4);
  }
}

#if !PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

// shared_ptr does not support volatile, so only test on no volatile
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

  l.register_member_ptr("aptr", &Bval::a);
  l.register_member_cptr("acptr", &Bval::a);

  l.register_member_ref("aref", &Bval::a);
  l.register_member_cref("acref", &Bval::a);

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
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 1);

  // can't modify b.a.i by b.a.i
  EXPECT_EQ(l.dostring("b.a.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 1);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 1);

  // modify b.a.i by aptr
  EXPECT_EQ(l.dostring("b.aptr.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 2);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 2);

  // can't modify b.a.i by acptr
  EXPECT_NE(l.dostring("b.acptr.i = 3"), LUA_OK);
  l.log_error_out();

  // modify b.a.i by aref
  EXPECT_EQ(l.dostring("b.aref.i = 4"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 4);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 4);

  // can't modify b.a.i by acref
  EXPECT_NE(l.dostring("b.acref.i = 5"), LUA_OK);
  l.log_error_out();
#else

  EXPECT_EQ(l.eval<int>("return b.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.aptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.acptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.aref.i"), 0);
  EXPECT_EQ(l.eval<int>("return b.acref.i"), 0);

  EXPECT_EQ(l.gettop(), 0);

  EXPECT_NE(l.dostring("b.i = 2"), LUA_OK);
  l.log_error_out();

  EXPECT_NE(l.dostring("b.aptr.i = 3"), LUA_OK);
  l.log_error_out();

  EXPECT_NE(l.dostring("b.acptr.i = 4"), LUA_OK);
  l.log_error_out();

  EXPECT_NE(l.dostring("b.aref.i = 3"), LUA_OK);
  l.log_error_out();

  EXPECT_NE(l.dostring("b.acref.i = 4"), LUA_OK);
  l.log_error_out();
  EXPECT_EQ(l.gettop(), 0);
#endif
}

struct C {
  volatile Bval b;
};

TEST(nested_objects, member_volatile_ptr) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("a", &Bval::a);
  l.register_member("i", &Bval::i);
  l.register_member("b", &C::b);

  l.register_member_ptr("aptr", &Bval::a);
  l.register_member_ptr("bptr", &C::b);

  auto c = std::make_shared<C>();
  l.set("c", c);

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

  // access c.b.i
  EXPECT_EQ(l.eval<int>("return c.b.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 1);

  // access c.b.a.i
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 1);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 1);

  // modify c.b.i by bptr
  EXPECT_EQ(l.dostring("c.bptr.i = 3;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.i"), 3);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 3);

  // modify c.b.a.i by bptr.aptr
  EXPECT_EQ(l.dostring("c.bptr.aptr.i = 4;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 4);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 4);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 4);

#else

  // access volatile b, got nil
  EXPECT_EQ(l.eval<int>("return c.b.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), 0);

  EXPECT_EQ(l.eval<int>("return c.b.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), 0);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), 0);

#endif

  EXPECT_EQ(l.gettop(), 0);
}

TEST(nested_objects, member_volatile_ref) {
  peacalm::luaw l;
  l.register_member("i", &A::i);
  l.register_member("a", &Bval::a);
  l.register_member("i", &Bval::i);
  l.register_member("b", &C::b);

  l.register_member_ptr("aptr", &Bval::a);
  l.register_member_ptr("bptr", &C::b);

  l.register_member_ref("aref", &Bval::a);
  l.register_member_ref("bref", &C::b);

  auto c = std::make_shared<C>();
  l.set("c", c);

  int ival;

#if PEACALM_LUAW_SUPPORT_VOLATILE_OBJECT

  // access c.b.i
  ival = 1;
  EXPECT_EQ(l.eval<int>("return c.b.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.i"), ival);

  // access c.b.a.i
  ival = 1;
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aref.i"), ival);

  // modify c.b.i by bptr
  ival = 2;
  EXPECT_EQ(l.dostring("c.bptr.i = 2;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.i"), ival);

  // modify c.b.a.i by bptr.aptr
  ival = 3;
  EXPECT_EQ(l.dostring("c.bptr.aptr.i = 3;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aref.i"), ival);

  // modify c.b.a.i by bptr.aref
  ival = 4;
  EXPECT_EQ(l.dostring("c.bptr.aref.i = 4;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aref.i"), ival);

  // modify c.b.a.i by bref.aptr
  ival = 5;
  EXPECT_EQ(l.dostring("c.bref.aptr.i = 5;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aref.i"), ival);

  // modify c.b.a.i by bref.aref
  ival = 6;
  EXPECT_EQ(l.dostring("c.bref.aref.i = 6;"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return c.b.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aref.i"), ival);
#else

  // access volatile b, got nil
  ival = 0;
  EXPECT_EQ(l.eval<int>("return c.b.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.i"), ival);

  EXPECT_EQ(l.eval<int>("return c.b.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.b.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bptr.aref.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.a.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aptr.i"), ival);
  EXPECT_EQ(l.eval<int>("return c.bref.aref.i"), ival);

#endif

  EXPECT_EQ(l.gettop(), 0);
}

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

TEST(nested_objects, nested_dynamic_class_member) {
  luaw l;
  l.register_ctor<A()>("NewA");
  l.register_member("i", &A::i);
  l.register_dynamic_member(foo_dm_getter, foo_dm_setter);

  auto o = std::make_shared<Foo>();
  l.set("o", o);

  EXPECT_EQ(l.dostring("o.a = NewA()"), LUA_OK);
  EXPECT_EQ(l.eval_int("return o.a.i"), 1);
  EXPECT_EQ(l.dostring("o.a.i = 2"), LUA_OK);
  EXPECT_EQ(l.eval_int("return o.a.i"), 2);

  // "b" and "o.b" share a same underlying object
  EXPECT_EQ(l.dostring("b = NewA(); b.i = 3; o.b = b"), LUA_OK);
  EXPECT_EQ(l.eval_int("return b.i"), 3);
  EXPECT_EQ(l.eval_int("return o.b.i"), 3);
  // modify by o.b.i
  EXPECT_EQ(l.dostring("o.b.i = 4"), LUA_OK);
  EXPECT_EQ(l.eval_int("return b.i"), 4);
  EXPECT_EQ(l.eval_int("return o.b.i"), 4);
  // modify by b.i
  EXPECT_EQ(l.dostring("b.i = 5"), LUA_OK);
  EXPECT_EQ(l.eval_int("return b.i"), 5);
  EXPECT_EQ(l.eval_int("return o.b.i"), 5);

  // get dynamic members
  {
    A a = l.eval<A>("return o.a");
    EXPECT_EQ(a.i, 2);
    A b = l.eval<A>("return o.b");
    EXPECT_EQ(b.i, 5);
  }
  {
    std::map<std::string, A> m;
    for (auto& p : o->m) {
      auto v = l.get<A>({"o", p.first});
      m.insert({p.first, v});
    }
    EXPECT_EQ(m["a"].i, 2);
    EXPECT_EQ(m["b"].i, 5);
  }

  // test by pointers
  {
    // set a pointer to o by wrapper
    Foo* p = o.get();
    l.set_ptr_by_wrapper("p", p);
    EXPECT_EQ(l.eval_int("return p.a.i"), 2);
    EXPECT_EQ(l.get_int({"p", "a", "i"}), 2);
    EXPECT_EQ(l.eval<A>("return p.a").i, 2);
    EXPECT_EQ(l.get<A>({"p", "a"}).i, 2);

    EXPECT_EQ(l.eval_int("return p.b.i"), 5);
    EXPECT_EQ(l.dostring("p.b.i = 6"), LUA_OK);
    EXPECT_EQ(l.eval_int("return p.b.i"), 6);

    // set a raw pointer to o
    l.set("rp", p);  // Not recommended usage!
    EXPECT_EQ(l.eval_int("return rp.a.i"), 2);
    EXPECT_EQ(l.get_int({"rp", "a", "i"}), 2);
    EXPECT_EQ(l.eval<A>("return rp.a").i, 2);
    EXPECT_EQ(l.get<A>({"rp", "a"}).i, 2);

    EXPECT_EQ(l.eval_int("return rp.b.i"), 6);
    EXPECT_EQ(l.dostring("rp.b.i = 7"), LUA_OK);
    EXPECT_EQ(l.eval_int("return rp.b.i"), 7);

    // set a const pointer to o by wrapper
    const Foo* cp = o.get();
    l.set_ptr_by_wrapper("cp", cp);
    EXPECT_EQ(l.eval_int("return cp.a.i"), 2);
    EXPECT_EQ(l.get_int({"cp", "a", "i"}), 2);
    EXPECT_EQ(l.eval<A>("return cp.a").i, 2);
    EXPECT_EQ(l.get<A>({"cp", "a"}).i, 2);

    EXPECT_EQ(l.eval_int("return cp.b.i"), 7);
    EXPECT_EQ(l.dostring("cp.b.i = 8"), LUA_OK);
    EXPECT_EQ(l.eval_int("return cp.b.i"), 8);
    EXPECT_NE(l.dostring("cp.b = NewA()"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return cp.b.i"), 8);
    EXPECT_NE(l.dostring("cp.c = NewA()"), LUA_OK);
    l.log_error_out();

    // set a raw const pointer to o
    l.set("rcp", cp);
    EXPECT_EQ(l.eval_int("return rcp.a.i"), 2);
    EXPECT_EQ(l.get_int({"rcp", "a", "i"}), 2);
    EXPECT_EQ(l.eval<A>("return rcp.a").i, 2);
    EXPECT_EQ(l.get<A>({"rcp", "a"}).i, 2);

    EXPECT_EQ(l.eval_int("return rcp.b.i"), 8);
    EXPECT_NE(l.dostring("rcp.b = NewA()"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return rcp.b.i"), 8);
    EXPECT_NE(l.dostring("rcp.c = NewA()"), LUA_OK);
    l.log_error_out();

    // set low-level const shared_ptr to o
    std::shared_ptr<const Foo> sco(o);
    l.set("sco", sco);
    EXPECT_EQ(l.eval_int("return sco.a.i"), 2);
    EXPECT_EQ(l.get_int({"sco", "a", "i"}), 2);
    EXPECT_EQ(l.eval<A>("return sco.a").i, 2);
    EXPECT_EQ(l.get<A>({"sco", "a"}).i, 2);

    EXPECT_EQ(l.eval_int("return sco.b.i"), 8);
    EXPECT_NE(l.dostring("sco.b = NewA()"), LUA_OK);
    l.log_error_out();
    EXPECT_EQ(l.eval_int("return sco.b.i"), 8);
    EXPECT_NE(l.dostring("sco.c = NewA()"), LUA_OK);
    l.log_error_out();
  }
}

}  // namespace
