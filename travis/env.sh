#!/bin/sh

set -eux

export CACHE_DIR=$HOME/cache
export CFLAGS="-I$CACHE_DIR/usr/include"
export LDFLAGS="-L$CACHE_DIR/usr/lib"
export LD_LIBRARY_PATH="$CACHE_DIR/usr/lib"
export PKG_CONFIG_PATH="$CACHE_DIR/usr/lib/pkgconfig"
