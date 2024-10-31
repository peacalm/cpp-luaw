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
  int plus() { return ++i; }
};

struct ObjDeleter {
  void operator()(Obj* p) const {
    ++cnt;
    delete p;
  }
  static std::atomic_int cnt;
};
std::atomic_int ObjDeleter::cnt{0};

TEST(unique_ptr_with_deleter, custom_deleter) {
  {
    EXPECT_EQ(ObjDeleter::cnt, 0);
    {
      std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
      EXPECT_EQ(ObjDeleter::cnt, 0);
    }
    EXPECT_EQ(ObjDeleter::cnt, 1);
    ObjDeleter::cnt = 0;
  }
  {
    luaw l;
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
    l.set("o", std::move(o));

    EXPECT_EQ(l.get_int({"o", "i"}), 1);
    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return o.i"), 2);

    l.close();  // call deleter
    EXPECT_EQ(ObjDeleter::cnt, 1);
  }
  ObjDeleter::cnt = 0;
  {
    luaw l;
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
    l.set("o", &o);

    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return o.i"), 2);

    l.close();
    EXPECT_EQ(ObjDeleter::cnt, 0);
  }
  ObjDeleter::cnt = 0;
  {
    luaw l;
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    std::unique_ptr<Obj, ObjDeleter> o(new Obj, ObjDeleter{});
    l.set("o", std::move(o));

    EXPECT_EQ(l.eval<int>("return o.i"), 1);
    EXPECT_EQ(l.eval<int>("return o:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return o.i"), 2);

    l.set("p", std::unique_ptr<Obj, ObjDeleter>(new Obj, ObjDeleter{}));

    EXPECT_EQ(l.eval<int>("return p.i"), 1);
    EXPECT_EQ(l.eval<int>("return p:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return p.i"), 2);

    l.close();
    EXPECT_EQ(ObjDeleter::cnt, 2);
  }
}

TEST(unique_ptr_with_deleter, mix_use_custom_and_default_deleter) {
  ObjDeleter::cnt = 0;
  luaw l;
  {
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    auto                             a = std::make_unique<Obj>();
    std::unique_ptr<Obj, ObjDeleter> b(new Obj, ObjDeleter{});

    EXPECT_EQ(ObjDeleter::cnt, 0);

    l.set("a", std::move(a));
    l.set("b", std::move(b));

    EXPECT_EQ(l.eval<int>("return a.i"), 1);
    EXPECT_EQ(l.eval<int>("return a:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return a.i"), 2);

    EXPECT_EQ(l.eval<int>("return b.i"), 1);
    EXPECT_EQ(l.eval<int>("return b:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return b.i"), 2);

    l.close();
    EXPECT_EQ(ObjDeleter::cnt, 1);
  }
  EXPECT_EQ(ObjDeleter::cnt, 1);

  ObjDeleter::cnt = 0;
  {
    l.reset();
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    auto                             a = std::make_unique<Obj>();
    std::unique_ptr<Obj, ObjDeleter> b(new Obj, ObjDeleter{});
    l.set("a", std::move(a));
    l.set("b", &b);

    EXPECT_EQ(l.eval<int>("return a.i"), 1);
    EXPECT_EQ(l.eval<int>("return a:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return a.i"), 2);

    EXPECT_EQ(l.eval<int>("return b.i"), 1);
    EXPECT_EQ(l.eval<int>("return b:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return b.i"), 2);

    l.close();
    EXPECT_EQ(ObjDeleter::cnt, 0);
  }
  EXPECT_EQ(ObjDeleter::cnt, 1);

  ObjDeleter::cnt = 0;
  {
    l.reset();
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    auto                             a = std::make_unique<Obj>();
    std::unique_ptr<Obj, ObjDeleter> b(new Obj, ObjDeleter{});
    l.set("a", &a);
    l.set("b", &b);

    EXPECT_EQ(l.eval<int>("return a.i"), 1);
    EXPECT_EQ(l.eval<int>("return a:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return a.i"), 2);

    EXPECT_EQ(l.eval<int>("return b.i"), 1);
    EXPECT_EQ(l.eval<int>("return b:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return b.i"), 2);

    l.close();
    EXPECT_EQ(ObjDeleter::cnt, 0);
  }
  EXPECT_EQ(ObjDeleter::cnt, 1);

  ObjDeleter::cnt = 0;
  {
    l.reset();
    l.register_member("i", &Obj::i);
    l.register_member("plus", &Obj::plus);

    auto                             a = std::make_unique<Obj>();
    std::unique_ptr<Obj, ObjDeleter> b(new Obj, ObjDeleter{});
    l.set("b", &b);
    l.set("a", &a);

    EXPECT_EQ(l.eval<int>("return a.i"), 1);
    EXPECT_EQ(l.eval<int>("return a:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return a.i"), 2);

    EXPECT_EQ(l.eval<int>("return b.i"), 1);
    EXPECT_EQ(l.eval<int>("return b:plus()"), 2);
    EXPECT_EQ(l.eval<int>("return b.i"), 2);

    l.close();
    EXPECT_EQ(ObjDeleter::cnt, 0);
  }
  EXPECT_EQ(ObjDeleter::cnt, 1);
}

}  // namespace
