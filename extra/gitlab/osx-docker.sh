#!/bin/sh
set -eu

export GL_BUILD="macos"

. ./extra/gitlab/env.sh

mkdir -p ${CACHE_DIR}/usr/lib
mkdir -p ${CACHE_DIR}/usr/include

export TARGET_HOST="--host=x86_64-apple-darwin14"
export TARGET_TRGT="--target=x86_64-darwin14-gcc"

export CFLAGS="${CFLAGS} -m64 "
export CFLAGS="${CFLAGS} -I/workdir/cache/macos/usr/include "
export CFLAGS="${CFLAGS} -I${CACHE_DIR}/usr/include "
export CFLAGS="${CFLAGS} -isystem /workdir/cache/macos/usr/include "
export CFLAGS="${CFLAGS} -isystem /workdir/cache/macos/usr/include/opus "
export CFLAGS="${CFLAGS} -isysroot /usr/osxcross/bin/../SDK/MacOSX10.10.sdk "
export CFLAGS="${CFLAGS} -mmacosx-version-min=10.10 "
export CFLAGS="${CFLAGS} -m64 -arch x86_64 -DNDEBUG -O3 -Wall"

export LDFLAGS="${LDFLAGS} -L/workdir/cache/macos/usr/lib "
export LDFLAGS="${LDFLAGS} -L${CACHE_DIR}/usr/lib "
export LDFLAGS="${LDFLAGS} -isysroot /usr/osxcross/bin/../SDK/MacOSX10.10.sdk "
export LDFLAGS="${LDFLAGS} -mmacosx-version-min=10.10 -m64 -arch x86_64"

# install libsodium, needed for crypto
if ! [ -d libsodium ]; then
  git clone --depth=1 --branch=stable https://github.com/jedisct1/libsodium.git
fi
cd libsodium
git rev-parse HEAD > libsodium.sha
if ! ([ -f "$CACHE_DIR/libsodium.sha" ] && diff "$CACHE_DIR/libsodium.sha" libsodium.sha); then
  ./autogen.sh
  ./configure $TARGET_HOST \
              --prefix="$CACHE_DIR/usr" \
              --quiet
  # libtool is broken when it comes to spaces in vars, so we have to neuter them
  # I live in backslash escapement hell...
  # This is also why we can't use ../common/*
  find . -type f -exec sed -i 's/libsodium\\\\ 1.0.12/libnacl-str/g' {} +
  make -j8
  make install
  mv libsodium.sha "$CACHE_DIR/libsodium.sha"
fi
cd ..
# rm -rf libsodium

. ./extra/common/build_opus.sh
. ./extra/common/build_vpx.sh

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch=$TOXCORE_REPO_BRANCH $TOXCORE_REPO_URI toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  if [ -d _build ]; then
    rm -rf _build
  fi
  find . -type f -exec sed -i 's/BUILD_TOXAV FALSE/BUILD_TOXAV TRUE/g' {} +
  # OSXCROSS is trying to be fancy, it's also wrong.
  # We can't use a common build_toxcore.sh because we need to
  # be able to set our own PKG_CONFIG_EXE
  cmake -B_build -H. \
        -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr \
        -DENABLE_STATIC=ON \
        -DENABLE_SHARED=OFF \
        -DCMAKE_SYSTEM_NAME=Darwin \
        -DBUILD_TOXAV=ON \
        -DPKG_CONFIG_EXECUTABLE=/usr/bin/pkg-config
  # mkdir _build
  # autoreconf -fi
  # (cd _build && ../configure --prefix=$CACHE_DIR/usr)
  make -C_build  || make -C_build VERBOSE=1
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
# rm -rf toxcore

ls -la $CACHE_DIR

cmake . -DCMAKE_SYSTEM_NAME=Darwin \
        -DENABLE_ASAN=OFF \
        -DFILTER_AUDIO=OFF \
        -DUTOX_STATIC=OFF \
        -DTOXCORE_STATIC=ON \
        -DENABLE_TESTS=OFF \
        -DENABLE_WERROR=OFF
make
