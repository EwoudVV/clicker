#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p ../work ../verification/2026-10-03
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I libraries/Clicker/src tests/state_test.cpp -o ../work/state-test
../work/state-test > ../verification/2026-10-03/state-tests.txt
cat ../verification/2026-10-03/state-tests.txt
