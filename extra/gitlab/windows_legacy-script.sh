#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

export CFLAGS="-I$CACHE_DIR/usr/include -I/usr/share/mingw-w64/include/ "

rm -rf build_win || true
mkdir build_win
cd build_win
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-win32.cmake \
    -DWIN_XP_LEGACY=ON \
    -DUTOX_STATIC=ON \
    -DTOXCORE_STATIC=ON \
    -DENABLE_TESTS=OFF \
    -DENABLE_WERROR=OFF
make || make VERBOSE=1
