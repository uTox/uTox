#!/bin/sh
set -eux

. ./travis/env.sh

cmake . -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-win64.cmake
make
