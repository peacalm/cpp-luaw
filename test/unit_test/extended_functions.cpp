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

TEST(extended_functions, IF) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_int("return IF(true, 1, 2)"), 1);
  EXPECT_EQ(l.eval_int("return IF(false, 1, 2)"), 2);
  EXPECT_EQ(l.eval_int("return IF(false, 1, '2')"), 2);
  EXPECT_EQ(l.eval_int("return IF(false, 1, '2.5')"), 2);
  EXPECT_EQ(l.eval_int("return IF(false, 1, 2.5)"), 2);
  EXPECT_EQ(l.eval_int("return IF(nil, 1, 2)"), 2);
  EXPECT_EQ(l.eval_int("return IF(1<0, 1, true, 3, 4)"), 3);
  EXPECT_EQ(l.eval_int("return IF(true and false, 1, nil, 3, 4)"), 4);
  EXPECT_EQ(l.eval_string("return IF(false, 1, nil, 3, 4)"), "4");
  EXPECT_EQ(l.eval_string("return IF(0, 'one', nil, 3, 4)"), "one");
  EXPECT_EQ(l.eval_string("return IF(not 0, 'one', 2^3 >= 8, 'three', 4)"),
            "three");
  EXPECT_EQ(l.eval_string("return IF(0>0, 'one', 2.5)"), "2.5");

  EXPECT_EQ(l.gettop(), 0);
}

TEST(extended_functions, SET) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[1]"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[3]"), false);
  EXPECT_EQ(l.eval_bool("return SET{1,2,4}[4]"), true);
  EXPECT_EQ(l.eval_bool("return SET{1.1,2.2}[2.2]"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[1] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[3] == nil"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,4)[3] ~= true"), true);
  EXPECT_EQ(l.eval_bool("return SET{1,2,4}[4] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET{1.1 ,2.2}[2.2] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,'x','y',4)['x'] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,2,'x','y',4)['z'] == nil"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,nil,'x','y',4)['x'] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET(1,nil,'x','y',4)['z'] == nil"), true);
  EXPECT_EQ(l.eval_bool("return SET{1,nil,'x','y',4}['x'] == true"), true);
  EXPECT_EQ(l.eval_bool("return SET{1,nil,'x','y',4}['z'] == nil"), true);

  EXPECT_EQ(l.eval_bool("s={1,2,4} return SET(s)[4] == true"), true);
  EXPECT_EQ(l.eval_bool("s={1,nil,'x','y',4} return SET(s)['x'] == true"),
            true);

  EXPECT_EQ(l.eval_bool("s=SET{1,2,4} return s[4] == true"), true);
  EXPECT_EQ(l.eval_bool("s=SET(1,nil,'x','y',4) return s.x == true"), true);
}

TEST(extended_functions, COUNTER) {
  lua_wrapper l;
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4)[1]"), 1);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,1,2,1)[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,1,2,1)[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,1,2,1)[2]"), 2);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,nil,1,2,1)[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,nil,1,2,1)[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER(1,2,4,nil,1,2,1)[2]"), 2);

  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,1,2,1}[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,1,2,1}[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,1,2,1}[2]"), 2);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,nil,1,2,1}[1]"), 3);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,nil,1,2,1}[3]"), 0);
  EXPECT_EQ(l.eval_int("return COUNTER{1,2,4,nil,1,2,1}[2]"), 2);

  EXPECT_EQ(l.eval_int("c={1,2,4,1,2,1} return COUNTER(c)[2]"), 2);
  EXPECT_EQ(l.eval_int("c={1,2,4,nil,1,2,1} return COUNTER(c)[1]"), 3);

  EXPECT_EQ(l.eval_int("c=COUNTER{1,2,4,1,2,1} return c[2] + c[4]"), 3);
  EXPECT_EQ(l.eval_int("c=COUNTER(1,2,4,nil,1,2,1) return c[1]"), 3);

  EXPECT_TRUE(l.eval_bool("return COUNTER(1,2,3)[4] == nil"));
}

TEST(extended_functions, COUNTER0) {
  lua_wrapper l;
  EXPECT_TRUE(l.eval_bool("return COUNTER0(1,2,3)[4] == 0"));
  EXPECT_EQ(l.eval_int("return COUNTER0(1,2,3)[4]"), 0);

  EXPECT_EQ(l.eval_int("return COUNTER0(1,2,4,1,2,1)[1]"), 3);
  EXPECT_TRUE(l.eval_bool("return COUNTER0(1,2,4,1,2,1)[3] == 0"));

  EXPECT_EQ(l.gettop(), 0);
}
