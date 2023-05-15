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

#include <gtest/gtest.h>

#if defined(ENABLE_MYOSTREAM_WATCH)
#include <myostream.h>
#define watch(...)                        \
  std::cout << MYOSTREAM_WATCH_TO_STRING( \
      std::string, " = ", "\n", "\n\n", __VA_ARGS__)
#define watch_with_std_cout(...) \
  MYOSTREAM_WATCH(std::cout, " = ", "\n", "\n\n", __VA_ARGS__)
#else
#define watch(...)
#define watch_with_std_cout(...)
#endif

#include "peacalm/lua_wrapper.h"

using namespace peacalm;
