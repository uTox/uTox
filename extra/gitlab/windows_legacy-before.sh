#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

export TARGET_HOST="--host=i686-w64-mingw32"
export TARGET_TRGT="--target=x86-win32-gcc"
export CROSS="i686-w64-mingw32-"

. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh
. ./extra/common/build_vpx.sh

# install toxcore
rm -rf toxcore
git clone --depth=1 --branch=$TOXCORE_REPO_BRANCH $TOXCORE_REPO_URI toxcore
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  rm -rf _build || true
  mkdir _build
  cmake -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ \
        -DCMAKE_RC_COMPILER=i686-w64-mingw32-windres \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_FIND_ROOT_PATH=$CACHE_DIR \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr \
        -DENABLE_SHARED=OFF \
        -DCOMPILE_AS_CXX=OFF \
        -DBOOTSTRAP_DAEMON=OFF \
        -B_build -H.
  make -C_build -j`nproc` || make -C_build VERBOSE=1
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore

. ./extra/common/build_openal.sh

export CC=i686-w64-mingw32-gcc
. ./extra/common/filter_audio.sh
i686-w64-mingw32-ranlib $CACHE_DIR/usr/lib/libfilteraudio.a
unset CC

cp $CACHE_DIR/usr/lib/libOpenAL32.a $CACHE_DIR/usr/lib/libopenal.a || true
# sudo curl https://cmdline.org/travis/32/shell32.a > $CACHE_DIR/usr/lib/libshell32.a
