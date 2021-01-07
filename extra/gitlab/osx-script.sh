#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

cmake . \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF \
  -DCMAKE_INCLUDE_PATH="$CACHE_DIR/usr/include" \
  -DCMAKE_LIBRARY_PATH="$CACHE_DIR/usr/lib" \
  -DENABLE_TESTS=OFF
  -DENABLE_WERROR=OFF
make VERBOSE=1
