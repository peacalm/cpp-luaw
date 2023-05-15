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

TEST(others, abs_index) {
  lua_wrapper l;
  EXPECT_EQ(l.gettop(), 0);
  EXPECT_EQ(l.abs_index(0), 0);
  EXPECT_EQ(l.abs_index(1), 1);
  EXPECT_EQ(l.abs_index(2), 2);
  EXPECT_EQ(l.abs_index(5), 5);
  EXPECT_EQ(l.abs_index(-1), -1);
  EXPECT_EQ(l.abs_index(-2), -2);
  EXPECT_EQ(l.abs_index(-5), -5);

  l.settop(3);
  EXPECT_EQ(l.abs_index(0), 0);
  EXPECT_EQ(l.abs_index(1), 1);
  EXPECT_EQ(l.abs_index(2), 2);
  EXPECT_EQ(l.abs_index(-1), 3);
  EXPECT_EQ(l.abs_index(-2), 2);
  EXPECT_EQ(l.abs_index(-3), 1);
  EXPECT_EQ(l.abs_index(-4), -4);
  EXPECT_EQ(l.abs_index(-5), -5);
}
