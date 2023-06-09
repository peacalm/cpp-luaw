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

#include <peacalm/luaw.h>

#include <cstdio>
#include <iostream>
#include <memory>

struct provider {
  provider() { puts("provider()"); }
  ~provider() { puts("~provider()"); }
  void provide(const std::vector<std::string> &vars, peacalm::luaw *l) {
    for (const auto &v : vars) provide(v, l);
  }
  void provide(const std::string &v, peacalm::luaw *l) {
    if (v == "a")
      l->set_integer(v, 1);
    else if (v == "b")
      l->set_integer(v, 2);
    else if (v == "c")
      l->set_integer(v, 3);
    else
      std::cout << "unknown: " << v << std::endl;
  }
};
using provider_type = std::unique_ptr<provider>;

int main() {
  peacalm::luaw_has_provider<provider_type> l;
  l.provider(std::make_unique<provider>());
  double ret = l.auto_eval_double("return a*10 + b^c");
  std::cout << ret << std::endl;
}
