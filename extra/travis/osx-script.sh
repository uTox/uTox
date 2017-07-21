#!/bin/sh
set -eux

. ./extra/travis/env.sh

cmake . -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib -DENABLE_WERROR=OFF
make || make VERBOSE=1
export ASAN_OPTIONS=use_odr_indicator=0
./run_tests.sh
export ASAN_OPTIONS=
