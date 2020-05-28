#!/usr/bin/env bash

set -eux

. ./extra/travis/env.sh

echo "======================================================"
echo "== DEBUGGING FAILURE "
echo "======================================================"
echo ""

echo ""
echo $HOME
echo $CACHE_DIR
ls -la $CACHE_DIR
echo ""
ls -la $CACHE_DIR/usr
echo ""

echo ""
echo $CFLAGS
echo $LDFLAGS
ls -la $LD_LIBRARY_PATH
echo ""

echo ""
ls -la $CACHE_DIR/usr/include
echo ""
ls -la $CACHE_DIR/usr/lib
echo ""
