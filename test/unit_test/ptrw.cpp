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
  int i = 0;
};

struct Bar {
  double i = 0;
};

TEST(ptrw, as_luaw_function_arg) {
  peacalm::luaw l;
  l.register_member("i", &Foo::i);
  l.register_member("i", &Bar::i);

  l.dostring(R"(
  f = function(a, b) 
    a.i = a.i + 1
    b.i = b.i + 100
  end
  )");

  Foo a;
  Bar b;
  l.callf<void>("f", &a, l.make_ptrw(&b));
  EXPECT_EQ(a.i, 1);
  EXPECT_EQ(b.i, 100);

  auto c = l.make_ptrw(&a);  // refer to a
  auto d = std::make_shared<Bar>();
  l.callf<void>("f", c, d);
  EXPECT_EQ(c->i, 2);
  EXPECT_EQ(a.i, 2);
  EXPECT_EQ(d->i, 100);

  // Get the function
  auto f = l.get<peacalm::luaw::function<void(peacalm::luaw::ptrw<Foo>,
                                              std::shared_ptr<Bar>)>>("f");
  f(c, d);
  EXPECT_FALSE(f.failed());
  EXPECT_EQ(c->i, 3);
  EXPECT_EQ(a.i, 3);
  EXPECT_EQ(d->i, 200);
}

}  // namespace
