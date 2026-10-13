#!/bin/sh
set -eux

. ./extra/ci/env.sh

export TARGET_HOST="--host=x86_64-w64-mingw32"
export TARGET_TRGT="--target=x86_64-win64-gcc"
export CROSS="x86_64-w64-mingw32-"

. ./extra/ci/common/build_nacl.sh
. ./extra/ci/common/build_opus.sh
. ./extra/ci/common/build_vpx.sh

# Newlines (not \-continuations) so word-splitting yields separate -D flags.
TOXCORE_CMAKE_ARGS="-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
-DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres
-DCMAKE_SYSTEM_NAME=Windows
-DCMAKE_FIND_ROOT_PATH=${CACHE_DIR}/usr
-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
-DCMAKE_PREFIX_PATH=${CACHE_DIR}/usr
-DENABLE_SHARED=OFF
-DENABLE_STATIC=ON
-DBUILD_TOXAV=ON
-DMUST_BUILD_TOXAV=ON
-DBUILD_MISC_TESTS=OFF
-DAUTOTEST=OFF
-DUNITTEST=OFF"
export TOXCORE_CMAKE_ARGS
. ./extra/ci/common/build_toxcore.sh

. ./extra/ci/common/build_openal.sh
. ./extra/ci/common/fix_openal_headers.sh

# MinGW OpenAL Soft installs as libOpenAL32.a; some find modules look for libopenal.a.
cp "$CACHE_DIR/usr/lib/libOpenAL32.a" "$CACHE_DIR/usr/lib/libopenal.a" || true
if [ ! -f "$CACHE_DIR/usr/lib/libOpenAL32.a" ] && [ -f "$CACHE_DIR/usr/lib/libopenal.a" ]; then
  cp "$CACHE_DIR/usr/lib/libopenal.a" "$CACHE_DIR/usr/lib/libOpenAL32.a"
fi

export CC=x86_64-w64-mingw32-gcc
. ./extra/ci/common/filter_audio.sh
x86_64-w64-mingw32-ranlib "$CACHE_DIR/usr/lib/libfilteraudio.a"
unset CC
