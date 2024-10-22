// Copyright (c) 2024 Li Shuangquan. All Rights Reserved.
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

#include <vector>

int main() {
  peacalm::luaw l;

  using VD       = std::vector<double>;
  using SharedVD = std::shared_ptr<VD>;

  // Constructor
  l.set("NewVD", []() { return std::make_shared<VD>(); });

  l.register_member<const double& (VD::*)(size_t) const>("at", &VD::at);
  // Fake member function: to stimulate the operator[]
  l.register_member<void (VD::*)(int, double)>(
      "set_at", [](VD* p, int idx, double v) { (*p)[idx] = v; });

  l.register_member("size", &VD::size);
  l.register_member("empty", &VD::empty);

  l.register_member("clear", &VD::clear);
  // Fake member function: to stimulate insert
  l.register_member<void (VD::*)(int, double)>(
      "insert_at", [](VD* p, int idx, double v) {
        p->insert(std::next(p->begin(), idx), v);
      });

  l.register_member<void (VD::*)(const double&)>("push_back", &VD::push_back);
  l.register_member("pop_back", &VD::pop_back);

  l.register_member<void (VD::*)(size_t)>("resize", &VD::resize);

  int retcode = l.dofile("test.lua");
  if (retcode != LUA_OK) {
    l.log_error_out();
    return retcode;
  }
  return 0;
}