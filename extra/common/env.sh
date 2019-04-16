#!/bin/sh

export CTEST_OUTPUT_ON_FAILURE=1

export CACHE_DIR=${CACHE_DIR=""}

export TOXCORE_REPO=${TOXCORE_REPO:-"TokTok/c-toxcore"}
export TOXCORE_REPO_URI="https://github.com/TokTok/c-toxcore.git"
export TOXCORE_REPO_BRANCH=${TOXCORE_REPO_BRANCH:-"master"}

export CFLAGS="-I${CACHE_DIR}/usr/include -I${CACHE_DIR}/usr/include/opus"
export LDFLAGS="-L${CACHE_DIR}/usr/lib"

export LD_LIBRARY_PATH="${CACHE_DIR}/usr/lib:/usr/lib"

export PKG_CONFIG_PATH="${CACHE_DIR}/usr/lib/pkgconfig"

# Cross compilation default targets.
export TARGET_HOST=${TARGET_HOST:-""}
export TARGET_TRGT=${TARGET_TRGT:-""}

export MAKEFLAGS="-j8"

# Used for different builds with the same target
export CMAKE_EXTRA_ARGS=""
