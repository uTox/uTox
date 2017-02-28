#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

export CFLAGS="-I$CACHE_DIR/usr/include -I/usr/share/mingw-w64/include/ "

mkdir build_win
cd build_win
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-win32.cmake -DWIN_XP_LEGACY=ON -DENABLE_TESTS=OFF -DENABLE_WERROR=OFF
make $MAKEFLAGS || make $MAKEFLAGS VERBOSE=1
