// Copyright (c) 2023-2024 Li Shuangquan. All Rights Reserved.
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

#include <peacalm/luaw.h>

#include <iostream>

int main() {
  peacalm::luaw l;

  if (l.dostring("a = 1 b = math.pi c = 10^12 + 123 d = 'good'") != LUA_OK) {
    l.log_error_in_stack();
    return 1;
  }

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
