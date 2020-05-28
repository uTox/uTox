#!/bin/sh
set -e -u -x

. ./extra/gitlab/env.sh

mkdir build
cd build
cmake .. -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib \
         -DSTATIC_ALL=ON \
         -DSTATIC_TOXCORE=ON \
         -DCMAKE_COLOR_MAKEFILE=ON \
         -DENABLE_TESTS=ON \
         -DENABLE_WERROR=ON \
         -DENABLE_ASAN=OFF \
         -DENABLE_FILTERAUDIO=OFF \
         -DENABLE_DBUS=OFF
make || make VERBOSE=1
