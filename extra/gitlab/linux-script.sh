#!/bin/sh
set -e -u -x

. ./extra/gitlab/env.sh

mkdir build
cd build
cmake .. \
    -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib \
    -DCMAKE_COLOR_MAKEFILE=ON \
    -DENABLE_TESTS=ON \
    -DENABLE_AUTOUPDATE=ON \
    -DENABLE_WERROR=ON
make
