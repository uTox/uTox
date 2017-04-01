#!/bin/sh

export CACHE_DIR=${CACHE_DIR=""}

export TOXCORE_REPO=${TOXCORE_REPO:-"TokTok/c-toxcore"}
export TOXCORE_REPO_URI="https://github.com/TokTok/c-toxcore.git"
export TOXCORE_REPO_BRANCH=${TOXCORE_REPO_BRANCH:-"v0.1.4"}

# export TOXCORE_REPO_URI="https://gitlab.com/Toxcore/toxcore.git"
#export TOXCORE_REPO_BRANCH="master"

export CFLAGS="-I${CACHE_DIR}/usr/include -I${CACHE_DIR}/usr/include/opus"
export LDFLAGS="-L${CACHE_DIR}/usr/lib"

export LD_LIBRARY_PATH="${CACHE_DIR}/usr/lib"
export PKG_CONFIG_PATH="${CACHE_DIR}/usr/lib/pkgconfig"
export PKG_CONFIG_LIBDIR="${CACHE_DIR}/usr/lib/pkgconfig"

# Cross compilation default targets.
export TARGET_HOST=${TARGET_HOST:-""}
export TARGET_TRGT=${TARGET_TRGT:-""}

export MAKEFLAGS="-j8"
