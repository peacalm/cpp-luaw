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

TEST(lua_wrapper, template_type_conversion) {
  lua_wrapper l;
  lua_pushinteger(l.L(), 1);
  EXPECT_EQ(l.to<int>(), 1);
  EXPECT_EQ(l.to<long>(), 1);
  EXPECT_EQ(l.to<long long>(), 1);
  EXPECT_EQ(l.to<std::string>(), "1");
  // l.to<const char*>(); // building will fail

  {
    // failure conversions
    EXPECT_EQ((l.to<std::map<int, int>>()), (std::map<int, int>{}));
    lua_pushstring(l.L(), "abc");
    EXPECT_EQ((l.to<std::unordered_map<int, int>>()),
              (std::unordered_map<int, int>{}));
    lua_pushboolean(l.L(), true);
    EXPECT_EQ((l.to<std::vector<int>>()), (std::vector<int>{}));
  }
  l.settop(0);

  // pair
  {
    EXPECT_EQ((l.to<std::pair<int, int>>()), (std::pair<int, int>{}));
    const char *expr = "t={1,2,3,4,x=1,y=2}";
    l.dostring(expr);
    l.getglobal("t");
    EXPECT_EQ((l.to<std::pair<int, int>>()), (std::pair<int, int>{1, 2}));
    EXPECT_EQ(l.gettop(), 1);

    l.dostring("t[1] = 2 t[2] = 3.5");
    EXPECT_EQ(l.gettop(), 1);
    EXPECT_EQ((l.to<std::pair<int, int>>()), (std::pair<int, int>{2, 3}));
    EXPECT_EQ((l.to<std::pair<int, double>>()),
              (std::pair<int, double>{2, 3.5}));
    EXPECT_EQ((l.to<std::pair<bool, double>>()),
              (std::pair<bool, double>{true, 3.5}));
    EXPECT_EQ((l.to<std::pair<std::string, double>>()),
              (std::pair<std::string, double>{"2", 3.5}));

    l.dostring("t[1] = nil t[2] = 3.5");
    EXPECT_EQ((l.to<std::pair<std::string, double>>()),
              (std::pair<std::string, double>{"", 3.5}));
    l.dostring("t[1] = nil t[2] = nil");
    EXPECT_EQ((l.to<std::pair<std::string, double>>()),
              (std::pair<std::string, double>{"", 0}));
    EXPECT_EQ((l.to<std::pair<bool, double>>()), (std::pair<bool, double>{}));

    l.dostring("t[1] = true t[2] = {1,2}");
    EXPECT_EQ((l.to<std::pair<bool, std::pair<int, int>>>()),
              (std::pair<bool, std::pair<int, int>>{true, {1, 2}}));

    l.dostring("t[1] = true t[2] = 2.5 t = {} ");
    EXPECT_EQ((l.to<std::pair<bool, double>>()),
              (std::pair<bool, double>{true, 2.5}));
    EXPECT_EQ((l.to<std::pair<long, double>>()),
              (std::pair<long, double>{1, 2.5}));
    EXPECT_EQ(l.gettop(), 1);

    l.getglobal("t");
    EXPECT_EQ((l.to<std::pair<bool, double>>()), (std::pair<bool, double>{}));
    EXPECT_EQ((l.to<std::pair<long, double>>()), (std::pair<long, double>{}));
    EXPECT_EQ(l.gettop(), 2);

    l.settop(0);
  }

  // vector
  {
    const char *expr = "t={1,2,3,4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>();
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1,2,'c'}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<std::string>>();
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<std::string>{"1", "2", "c"}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={a=1,b=2, 10,20}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>(-1);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{10, 20}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={{1,2},{3,4}}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<std::vector<int>>>(-1, true, nullptr);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<std::vector<int>>{{1, 2}, {3, 4}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, nil, nil, 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>(-1);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 4}));  // ignore nil
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto v  = l.to<std::vector<int>>(-1);
    watch(expr, v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    bool failed, exists;
    auto v = l.to<std::vector<std::string>>(-1, false, &failed, &exists);
    watch(expr, v, failed, exists);
    EXPECT_EQ(v, (std::vector<std::string>{"1", "2", "a", "b", "4"}));
    EXPECT_EQ(l.gettop(), sz);
  }

  // set
  {
    const char *expr = "t={1,2,3,4,3,2}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>();
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1,2,3,4,3}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::unordered_set<int>>();
    watch(expr, s);
    EXPECT_EQ(s, (std::unordered_set<int>{1, 2, 3, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1,3,4,3}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::unordered_set<std::string>>();
    watch(expr, s);
    EXPECT_EQ(s, (std::unordered_set<std::string>{"1", "3", "4"}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={a=1,b=2, 10,20}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{10, 20}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, nil, 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{1, 2, 4}));  // ignore nil
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, nil, nil, 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::unordered_set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::unordered_set<int>{1, 2, 4}));  // ignore nil
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    auto s  = l.to<std::set<int>>(-1);
    watch(expr, s);
    EXPECT_EQ(s, (std::set<int>{1, 2, 4}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "t={1, 2, 'a', 'b', 4}";
    l.dostring(expr);
    l.getglobal("t");
    int  sz = l.gettop();
    bool failed, exists;
    auto s = l.to<std::unordered_set<std::string>>(-1, false, &failed, &exists);
    watch(expr, s, failed, exists);
    EXPECT_EQ(s, (std::unordered_set<std::string>{"1", "2", "a", "b", "4"}));
    EXPECT_EQ(l.gettop(), sz);
  }

  // map
  {
    const char *expr = "m={a=1, b=2, c=3}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz       = l.gettop();
    using map_t  = std::map<std::string, long>;
    using umap_t = std::unordered_map<std::string, long>;
    auto m       = l.to<map_t>();
    auto um      = l.to<umap_t>();
    watch(expr, m, um);
    EXPECT_EQ(m, (map_t{{"a", 1}, {"b", 2}, {"c", 3}}));
    EXPECT_EQ(um, (umap_t{{"a", 1}, {"b", 2}, {"c", 3}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, b=nil, c=3}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz       = l.gettop();
    using map_t  = std::map<std::string, long>;
    using umap_t = std::unordered_map<std::string, long>;
    auto m       = l.to<map_t>();
    auto um      = l.to<umap_t>();
    watch(expr, m, um);
    EXPECT_EQ(m, (map_t{{"a", 1}, {"c", 3}}));
    EXPECT_EQ(um, (umap_t{{"a", 1}, {"c", 3}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, b=2, 3, 4}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz       = l.gettop();
    using map_t  = std::map<std::string, long>;
    using umap_t = std::unordered_map<std::string, long>;
    auto m       = l.to<map_t>(-1);
    auto um      = l.to<umap_t>(-1);
    watch(expr, m, um);
    EXPECT_EQ(m, (map_t{{"a", 1}, {"b", 2}, {"1", 3}, {"2", 4}}));
    EXPECT_EQ(um, (umap_t{{"a", 1}, {"b", 2}, {"1", 3}, {"2", 4}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, 1, 2, [3] = 3, [5]=5}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, int>>();
    auto um = l.to<std::unordered_map<int, int>>();
    watch(expr, m, um);
    EXPECT_EQ(m, (std::map<int, int>{{1, 1}, {2, 2}, {3, 3}, {5, 5}}));
    EXPECT_EQ(um,
              (std::unordered_map<int, int>{{1, 1}, {2, 2}, {3, 3}, {5, 5}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={[1]=1, [2]=2,  10, 20}";
    l.dostring(expr);
    l.getglobal("m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, int>>(-1, true, nullptr);
    auto um = l.to<std::unordered_map<int, int>>(-1, true, nullptr);
    watch(expr, m, um);
    EXPECT_EQ(m, (std::map<int, int>{{1, 10}, {2, 20}}));
    EXPECT_EQ(um, (std::unordered_map<int, int>{{1, 10}, {2, 20}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={10, 20, [1]=1, [2]=2}";
    l.dostring(expr);
    l.getglobal("m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, int>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m, (std::map<int, int>{{1, 10}, {2, 20}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1, b=2, 11, true, F=false}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, int>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, int>{
                  {"a", 1}, {"b", 2}, {"1", 11}, {"2", 1}, {"F", 0}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1.0, b=2.5, c=3, 1.1, 2.2, true, F=false}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, double>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, double>{{"a", 1},
                                             {"b", 2.5},
                                             {"c", 3},
                                             {"1", 1.1},
                                             {"2", 2.2},
                                             {"3", 1},
                                             {"F", 0}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a=1.0, b=2.5, c=3, 1.1, 2.2, true, F=false}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::unordered_map<std::string, double>>(-1, true, nullptr);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::unordered_map<std::string, double>{{"a", 1},
                                                       {"b", 2.5},
                                                       {"c", 3},
                                                       {"1", 1.1},
                                                       {"2", 2.2},
                                                       {"3", 1},
                                                       {"F", 0}}));
    EXPECT_EQ(l.gettop(), sz);
  }

  // complex
  {
    const char *expr = "m={a={1,3}, b={2,4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz   = l.gettop();
    auto m    = l.to<std::map<std::string, std::vector<int>>>(-1);
    auto mpii = l.to<std::map<std::string, std::pair<int, int>>>(-1);
    watch(expr, m, mpii);
    EXPECT_EQ(m,
              (std::map<std::string, std::vector<int>>{{"a", {1, 3}},
                                                       {"b", {2, 4}}}));
    EXPECT_EQ(mpii,
              (std::map<std::string, std::pair<int, int>>{{"a", {1, 3}},
                                                          {"b", {2, 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={[10010]={1,3}, [10086]={2,4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<std::string, std::vector<int>>>(-1);
    watch(expr, m);
    EXPECT_EQ(m,
              (std::map<std::string, std::vector<int>>{{"10010", {1, 3}},
                                                       {"10086", {2, 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={[10010]={1,3}, [10086]={2,4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int  sz = l.gettop();
    auto m  = l.to<std::map<int, std::vector<int>>>(-1);
    watch(expr, m);
    EXPECT_EQ(
        m, (std::map<int, std::vector<int>>{{10010, {1, 3}}, {10086, {2, 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "m={a={X=1,Y=3}, b={X=2,Y=4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "m");
    int sz     = l.gettop();
    using type = std::unordered_map<std::string, std::map<std::string, int>>;
    auto m     = l.to<type>(-1);
    watch(expr, m);
    EXPECT_EQ(m,
              (type{{"a", {{"X", 1}, {"Y", 3}}}, {"b", {{"X", 2}, {"Y", 4}}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
  {
    const char *expr = "p={{X=1,Y=3}, {X=2,Y=4}}";
    l.dostring(expr);
    lua_getglobal(l.L(), "p");
    int sz     = l.gettop();
    using type = std::pair<std::unordered_map<std::string, int>,
                           std::map<std::string, int>>;
    auto m     = l.to<type>(-1);
    watch(expr, m);
    EXPECT_EQ(m, (type{{{"X", 1}, {"Y", 3}}, {{"X", 2}, {"Y", 4}}}));
    EXPECT_EQ(l.gettop(), sz);
  }
}