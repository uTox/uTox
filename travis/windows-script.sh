#!/bin/sh
set -eux

. ./travis/env.sh



mkdir build_win
cd build_win
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-win64.cmake CFLAGS="-I$CACHE_DIR/usr/include -I/usr/share/mingw-w64/include/ "
make VERBOSE=1
