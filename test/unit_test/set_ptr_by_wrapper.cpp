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

struct Foo {
  int i = 1;
};
struct Bar {
  int i = 2;
};

TEST(set_ptr_by_wrapper, raw_ptr) {
  luaw l;
  Foo  f;
  Bar  b;

  l.register_member("i", &Foo::i);
  l.register_member("i", &Bar::i);

  l.set_ptr_by_wrapper("f", &f);
  l.set_ptr_by_wrapper("b", &b);

  l.gseek("f");
  std::string fmetatb = l.get_metatable_name(-1);
  l.gseek("b");
  std::string bmetatb = l.get_metatable_name(-1);
  l.pop(2);
  EXPECT_NE(fmetatb, bmetatb);

  l.set("rawf", &f);
  l.set("rawb", &b);
  l.gseek("rawf");
  std::string rawfmetatb = l.get_metatable_name(-1);
  l.gseek("rawb");
  std::string rawbmetatb = l.get_metatable_name(-1);
  l.pop(2);
  EXPECT_EQ(rawfmetatb, rawbmetatb);

  l.gseek("f");
  std::string fmetatb_again = l.get_metatable_name(-1);
  l.gseek("b");
  std::string bmetatb_again = l.get_metatable_name(-1);
  l.pop(2);
  EXPECT_EQ(fmetatb, fmetatb_again);
  EXPECT_EQ(bmetatb, bmetatb_again);

  watch(fmetatb, bmetatb, rawfmetatb, rawbmetatb, fmetatb_again, bmetatb_again);

  EXPECT_EQ(l.eval<int>("return f.i"), f.i);
  EXPECT_EQ(l.eval<int>("return b.i"), b.i);

  EXPECT_EQ(l.dostring("f.i = 3"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return f.i"), 3);
  EXPECT_EQ(f.i, 3);

  EXPECT_EQ(l.dostring("b.i = 4"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return b.i"), 4);
  EXPECT_EQ(b.i, 4);
}

TEST(set_ptr_by_wrapper, raw_ptr_add_const) {
  luaw l;
  Foo  f;

  l.register_member("i", &Foo::i);

  l.set_ptr_by_wrapper("f", &f);
  l.set_ptr_by_wrapper("cf", (const Foo*)(&f));

  l.gseek("f");
  std::string fmetatb = l.get_metatable_name(-1);
  l.gseek("cf");
  std::string cfmetatb = l.get_metatable_name(-1);
  l.pop(2);
  EXPECT_NE(fmetatb, cfmetatb);

  EXPECT_EQ(l.eval<int>("return f.i"), f.i);
  EXPECT_EQ(l.eval<int>("return cf.i"), f.i);

  EXPECT_EQ(l.dostring("f.i = 3"), LUA_OK);
  EXPECT_EQ(l.eval<int>("return f.i"), 3);
  EXPECT_EQ(f.i, 3);

  EXPECT_NE(l.dostring("cf.i = 4"), LUA_OK);
  l.log_error_out();

  const Foo c;
  l.set_ptr_by_wrapper("c", &c);

  l.gseek("c");
  std::string cmetatb = l.get_metatable_name(-1);
  l.pop();
  EXPECT_EQ(cfmetatb, cmetatb);

  watch(fmetatb, cfmetatb, cmetatb);

  EXPECT_NE(l.dostring("c.i = 5"), LUA_OK);
  l.log_error_out();
}

}  // namespace
