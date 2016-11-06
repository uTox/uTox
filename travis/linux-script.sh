#!/bin/sh
set -e -u -x

. ./travis/env.sh

echo "@grayhatter, you need to add FILTER_AUDIO support back in!!"

cmake .
make
