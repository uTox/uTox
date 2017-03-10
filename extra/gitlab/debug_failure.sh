#!/usr/bin/env bash

set -eux

. ./extra/gitlab/env.sh

echo "======================================================"
echo "== DEBUGING FAILURE "
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
