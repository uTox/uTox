#!/bin/sh
set -e -u -x

. ./extra/travis/env.sh

if [ -f /usr/bin/gcc-6 ]; then # the default version of gcc is broken on travis
    export CC=gcc-6
    export CFLAGS="-fuse-ld=gold"
fi

cmake . \
  -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/include" \
  -DCMAKE_LIBRARY_PATH="$CACHE_DIR/usr/lib" \
  -DENABLE_TESTS=ON -DENABLE_WERROR=ON \
  -DENABLE_DBUS=ON \
  -DENABLE_AUTOUPDATE=ON \
  "$CMAKE_EXTRA_ARGS"
make
./run_tests.sh
