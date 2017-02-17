#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

cmake . -DCMAKE_INCLUDE_PATH=$CACHE_DIR/usr/lib
make VERBOSE=1
