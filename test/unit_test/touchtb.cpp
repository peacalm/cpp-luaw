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

TEST(touchtb, touchtb) {
  lua_wrapper l;
  l.gtouchtb("g");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.istable());

  l.gtouchtb("g");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.istable());

  l.touchtb("gg");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_TRUE(l.istable());

  l.pop();
  l.touchtb("gg");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_TRUE(l.istable());

  l.settop(0);
}
