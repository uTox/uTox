#!/bin/sh -e

cd "@utoxTESTS_BINARY_DIR@"
# remove ./tox folder before each test to have clean environment
rm -rf ./tox
ctest -VV
