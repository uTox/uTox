#!/bin/sh
set -eux

. ./extra/travis/env.sh

cmake . \
  -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/include" \
  -DCMAKE_LIBRARY_PATH="$CACHE_DIR/usr/lib" \
  -DENABLE_WERROR=OFF
make VERBOSE=1
./run_tests.sh
