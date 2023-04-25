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

#include <peacalm/lua_wrapper.h>

#include <iostream>
#include <string>

int main() {
  peacalm::lua_wrapper l;
  l.set_integer("a", 10);
  l.set_integer("b", 5);
  l.set_integer("c", 2);
  std::string expr = "return a^2 + b/c";
  double      ret  = l.eval_double(expr);
  std::cout << "ret = " << ret << std::endl;  // 102.5
  std::string s =
      l.eval_string("if a > b + c then return 'good' else return 'bad' end");
  std::cout << "s = " << s << std::endl;  // good

  auto si = l.eval<std::set<int>>("return {a, b, c}");  // {2,5,10}
}