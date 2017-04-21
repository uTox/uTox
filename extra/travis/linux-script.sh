#!/bin/sh
set -e -u -x

. ./extra/travis/env.sh
pkg-config --cflags dbus-1
cmake . -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/lib" -DENABLE_TESTS=ON -DENABLE_WERROR=ON -DENABLE_DBUS=ON -DENABLE_AUTOUPDATE=ON
make
./run_tests.sh
