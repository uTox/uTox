#!/bin/sh

export CACHE_DIR=${CACHE_DIR=""}

export TOXCORE_REPO="TokTok/c-toxcore"

export CFLAGS="-I$CACHE_DIR/usr/include"
export LDFLAGS="-L$CACHE_DIR/usr/lib"

export LD_LIBRARY_PATH="$CACHE_DIR/usr/lib"
export PKG_CONFIG_PATH="$CACHE_DIR/usr/lib/pkgconfig"

# Cross compilation default targets.
export TARGET_HOST=""
