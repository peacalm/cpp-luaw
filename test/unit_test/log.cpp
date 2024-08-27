// Copyright (c) 2023 Li Shuangquan. All Rights Reserved.
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

TEST(log, disable_log) {
  luaw l;

  l.set_string("x", "enable log once then disable!");
  bool disable_log = false, failed = false, exists = false;
  EXPECT_EQ(l.get_bool("x", false, disable_log, &failed, &exists), false);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  disable_log = true;
  failed      = false;
  exists      = false;
  EXPECT_EQ(l.get_bool("x", false, disable_log, &failed, &exists), false);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  failed = false;
  exists = false;
  EXPECT_EQ(l.get_int("x", 0, disable_log, &failed, &exists), 0);
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  l.set_boolean("b", true);
  failed = false;
  exists = false;
  EXPECT_EQ(l.get_string("b", "", disable_log, &failed, &exists), "");
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);
  l.set_boolean("b", false);
  failed = false;
  exists = false;
  EXPECT_EQ(l.get_string("b", "", disable_log, &failed, &exists), "");
  EXPECT_TRUE(failed);
  EXPECT_TRUE(exists);

  EXPECT_EQ(l.gettop(), 0);
}