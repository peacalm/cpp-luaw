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

TEST(seek, seek) {
  luaw l;
  l.gseek("g");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.isnil(-1));
  l.seek("gg");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.isnil(-1));
  l.seek(1);
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_TRUE(l.isnil(-1));

  l.settop(0);
  l.dostring("g={a=1, gg={a=11,b='bb'}, list={1,2,3}}");
  l.gseek("g");
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_TRUE(l.istable(-1));
  l.seek("a");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_EQ(l.to<int>(), 1);
  l.pop();
  l.seek("gg");
  EXPECT_EQ(l.gettop(), 2);
  EXPECT_TRUE(l.istable(-1));
  l.seek("a");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_EQ(l.to<int>(), 11);
  l.pop();
  l.seek("b");
  EXPECT_EQ(l.gettop(), 3);
  EXPECT_EQ(l.to<std::string>(), "bb");
  l.pop(2);
  l.seek("list");
  int list_idx = l.gettop();
  EXPECT_EQ(l.seek(2).to_int(), 2);
  l.pop();
  EXPECT_EQ(l.seek(3).to<long>(), 3);
  EXPECT_EQ(l.seek(1, list_idx).to_double(), 1);
  EXPECT_EQ(l.gettop(), 4);

  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek("gg").seek("a").to_int(), 11);
  EXPECT_EQ(l.gseek("g").seek("list").seek(2).to_int(), 2);
  EXPECT_EQ(l.gettop(), 6);

  l.seek(3);
  EXPECT_TRUE(l.isnil());
  l.pop(2);
  l.seek(3);
  EXPECT_FALSE(l.isnil());
  EXPECT_TRUE(l.isinteger());
  EXPECT_TRUE(l.isnumber());
  EXPECT_EQ(l.to_uint(), 3);

  l.settop(0);
  EXPECT_EQ(l.lseek("g", "gg", "a").to_int(), 11);
  EXPECT_EQ(l.lseek("g", "list", 2).to_int(), 2);
  EXPECT_EQ(l.gettop(), 6);

  l.dostring("a = 1 b = 2");
  EXPECT_EQ(l.lseek("a").to_long(), 1);
  EXPECT_EQ(l.gseek("b").to_long(), 2);
  EXPECT_EQ(l.gettop(), 8);

  l.settop(0);
  l.dostring("g={{1,2,3.0}, {'a', 'b', 'c'}, m={{a=1},{a=2}} }");
  EXPECT_EQ(l.gseek("g").seek(1).seek(1).to_int(), 1);
  EXPECT_EQ(l.gettop(), 3);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).seek(3).to_int(), 3);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).seek(3).to_string(), "3.0");
  EXPECT_EQ(l.gettop(), 3);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(2).seek(3).to<std::string>(), "c");
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek("m").seek(1).seek("a").to_int(), 1);
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek("m").seek(2).seek("a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 4);

  l.settop(0);
  EXPECT_EQ(l.lseek("g", "m", 2, "a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 4);
  EXPECT_EQ(l.lseek("g", "m", 2, "a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 8);
  l.settop(0);
  EXPECT_EQ(l.gseek("_G").seek("g").seek("m").seek(2).seek("a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 5);
  EXPECT_EQ(l.gseek_env().seek("g").seek("m").seek(2).seek("a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 10);
  EXPECT_EQ(l.lseek("_G", "g", "m", 2, "a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 15);
  l.settop(0);
  EXPECT_EQ(l.lseek("_G", "_G", "_G", "g", "m", 2, "a").to_int(), 2);
  EXPECT_EQ(l.gettop(), 7);
  l.settop(0);
  EXPECT_EQ(
      l.gseek("_G").seek("_G").seek("g").seek("m").seek(2).seek("a").to_int(),
      2);
  EXPECT_EQ(l.gettop(), 6);

  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).to<std::vector<int>>(),
            (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(l.lseek("g", 1).to<std::vector<int>>(),
            (std::vector<int>{1, 2, 3}));
  l.settop(0);
  EXPECT_EQ(l.gseek("g").seek(1).to<std::vector<std::string>>(),
            (std::vector<std::string>{"1", "2", "3.0"}));
  EXPECT_EQ(l.gettop(), 2);

  l.pop();
  EXPECT_EQ(l.gettop(), 1);
  EXPECT_EQ((l.seek("m").seek(2).to<std::map<std::string, int>>()),
            (std::map<std::string, int>{{"a", 2}}));
  EXPECT_EQ(l.gettop(), 3);

  l.seek(3);
  EXPECT_TRUE(l.isnil());
  EXPECT_EQ(l.gettop(), 4);
  l.pop();

  l.settop(0);

  // nullptr
  l.gseek(nullptr);
  EXPECT_EQ(l.to_int(), 0);
  l.seek((const char*)nullptr);
  EXPECT_EQ(l.to_int(), 0);
  l.gseek(std::string{});
  EXPECT_EQ(l.to_int(), 0);
  l.gseek("g");
  l.seek((const char*)nullptr);
  EXPECT_EQ(l.to_int(), 0);

  l.gseek("g");
  // l.seek(nullptr);  // error
  const volatile std::nullptr_t cvnullptr = 0;
  // l.seek(cvnullptr);  // error

  l.settop(0);
}
