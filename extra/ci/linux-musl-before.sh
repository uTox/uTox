#!/bin/sh
set -eux

. ./extra/ci/env.sh

mkdir -p "$CACHE_DIR/usr/include" "$CACHE_DIR/usr/lib" "$CACHE_DIR/usr/lib/pkgconfig"

# Native musl build: PIC for static libs that end up in a shared-capable binary.
export TARGET_TRGT="--enable-pic"
export CFLAGS="${CFLAGS:-} -fPIC"

. ./extra/ci/common/build_nacl.sh
. ./extra/ci/common/build_opus.sh
. ./extra/ci/common/build_vpx.sh

# Newlines (not \-continuations) so word-splitting yields separate -D flags.
TOXCORE_CMAKE_ARGS="-DENABLE_STATIC=ON
-DENABLE_SHARED=OFF
-DBUILD_TOXAV=ON
-DMUST_BUILD_TOXAV=ON
-DBUILD_MISC_TESTS=OFF
-DAUTOTEST=OFF
-DUNITTEST=OFF
-DCMAKE_PREFIX_PATH=${CACHE_DIR}/usr"
export TOXCORE_CMAKE_ARGS
. ./extra/ci/common/build_toxcore.sh

. ./extra/ci/common/filter_audio.sh

OPENAL_INCLUDE_DIR=/usr/include
export OPENAL_INCLUDE_DIR
. ./extra/ci/common/fix_openal_headers.sh
