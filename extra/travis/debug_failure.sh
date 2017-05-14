#!/usr/bin/env bash

set -eux

. ./extra/travis/env.sh

echo "======================================================"
echo "== DEBUGING FAILURE "
echo "======================================================"
echo ""

cat "/Users/travis/build/uTox/uTox/CMakeFiles/utox.dir/link.txt"
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
