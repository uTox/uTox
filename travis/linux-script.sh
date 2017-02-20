#!/bin/sh
set -e -u -x

. ./travis/env.sh

echo "@grayhatter, you need to add FILTER_AUDIO support back in!!"

cmake . -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib
make
./run_tests.sh
