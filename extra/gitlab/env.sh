#!/bin/sh

set -eux

export TOXCORE_REPO=TokTok/c-toxcore

export CACHE_DIR=`pwd`/cache/$GL_BUILD
mkdir -p $CACHE_DIR || true

export CFLAGS="-I$CACHE_DIR/usr/include"
export LDFLAGS="-L$CACHE_DIR/usr/lib"

export LD_LIBRARY_PATH="$CACHE_DIR/usr/lib"
export PKG_CONFIG_PATH="$CACHE_DIR/usr/lib/pkgconfig"

export TERM=vt100
