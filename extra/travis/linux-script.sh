#!/bin/sh
set -e -u -x

. ./extra/travis/env.sh

cmake . \
  -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/include" \
  -DCMAKE_LIBRARY_PATH="$CACHE_DIR/usr/lib" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DENABLE_TESTS=ON \
  -DENABLE_WERROR="${ENABLE_WERROR:-ON}" \
  -DENABLE_DBUS=ON
make
./run_tests.sh
