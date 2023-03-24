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

int main() {
  peacalm::lua_wrapper l;

  // equals to: l.dostring("a = 1 b = math.pi c = 10^12 + 123 d = 'good'");
  l.dofile("conf.lua");

  int         a = l.get_int("a");
  double      b = l.get_double("b");
  long        c = l.get_llong("c");
  std::string d = l.get_string("d");
  std::cout << "a = " << a << std::endl;
  std::cout << "b = " << b << std::endl;
  std::cout << "c = " << c << std::endl;
  std::cout << "d = " << d << std::endl;
  std::cout << "nx = " << l.get_int("nx", -1) << std::endl;
}
