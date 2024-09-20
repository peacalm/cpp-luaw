#!/usr/bin/env bash
# Copyright (c) 2023 Li Shuangquan. All Rights Reserved.
#
# Licensed under the MIT License (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of the License
# at
#
#   http://opensource.org/licenses/MIT
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

cd $(dirname "$0") || exit

TEST=ON
MYOSTREAM=OFF
SEPARATE=OFF
NEED_VOLATILE=OFF
UTONLY=""

RUN_UNIT_TEST=0
RUN_PERFORMANCE_TEST=0
REBUILD=0
CLEAR=0

while [ "$#" -gt 0 ]; do
  case $1 in
    -t)
      TEST=$2
      shift
      shift
      ;;
    -ru)
      RUN_UNIT_TEST=1
      shift
      ;;
    -rp)
      RUN_PERFORMANCE_TEST=1
      shift
      ;;
    -r)
      RUN_UNIT_TEST=1
      RUN_PERFORMANCE_TEST=1
      shift
      ;;
    -o)
      MYOSTREAM=ON
      shift
      ;;
    --rebuild)
      REBUILD=1
      shift
      ;;
    --clear)
      CLEAR=1
      shift
      ;;
    --separate)
      SEPARATE=ON
      shift
      ;;
    --volatile)
      NEED_VOLATILE=ON
      shift
      ;;
    -utonly)
      UTONLY=$2
      shift
      shift
      ;;
    -h)
      echo "run_test.sh [-t UNIT/PERF] [-o] [-r|-ru|-rp] [--rebuild] [--clear]"\
           "[--separate] [--volatile] [-utonly <file>]"
      exit 0
      ;;
    *)
      echo "Invalid option '$1'"
      echo
      exit 1
      ;;
  esac
done

if [ ${REBUILD} -eq 1 ]; then
  rm -rf build
fi
mkdir -p build
cd build
cmake .. -DBUILD_TEST=${TEST} \
         -DENABLE_MYOSTREAM_WATCH=${MYOSTREAM} \
         -DUNIT_TEST_SEPARATE=${SEPARATE} \
         -DNEED_VOLATILE=${NEED_VOLATILE} \
         -DUTONLY=${UTONLY}
make

if [ $? -eq 0 ]; then
  ctest --output-on-failure
  if [ ${RUN_UNIT_TEST} -eq 1 ]; then
    if [ -n "${UTONLY}" ]; then
      ./test/unit_test/${UTONLY}
    else
      ./test/unit_test/unit_test
    fi
  fi
  if [ ${RUN_PERFORMANCE_TEST} -eq 1 ]; then
    ./test/perf_test/perf_test
  fi
else
  echo
  echo "Building Failed!"
fi

if [ ${CLEAR} -eq 1 ]; then
  cd ..
  rm -rf build
fi
