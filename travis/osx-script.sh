#!/bin/sh
set -eux

. ./travis/env.sh

cmake . -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib
make
