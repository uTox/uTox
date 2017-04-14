#!/bin/sh
set -e -u -x

. ./extra/travis/env.sh

cmake . -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/lib" -DENABLE_TESTS=ON -DENABLE_WERROR=ON -DENABLE_DBUS=OFF
make
./run_tests.sh || true
