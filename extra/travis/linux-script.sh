#!/bin/sh
set -e -u -x

. ./extra/travis/env.sh

echo "@grayhatter, you need to add FILTER_AUDIO support back in!!"

cmake . -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib -DENABLE_TESTS=ON -DENABLE_WERROR=ON
make $MAKEFLAGS
./run_tests.sh
