#!/bin/sh
set -e -u -x

. ./extra/gitlab/env.sh

echo "@grayhatter, you need to add FILTER_AUDIO support back in!!"

cmake . -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib
make
