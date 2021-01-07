#!/bin/sh
set -e -u -x

. ./extra/travis/env.sh

cmake . \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF \
  -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/include" \
  -DCMAKE_LIBRARY_PATH="$CACHE_DIR/usr/lib" \
  -DENABLE_TESTS=ON -DENABLE_WERROR=ON \
  -DENABLE_DBUS=ON
make
./run_tests.sh
