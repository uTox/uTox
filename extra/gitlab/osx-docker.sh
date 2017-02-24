#!/bin/sh

set -eux

export GL_BUILD="macos"

. ./extra/gitlab/env.sh

export CFLAGS=" -m64 -I/workdir/cache/macos/usr/include -isystem /workdir/cache/macos/usr/include -isystem /workdir/cache/macos/usr/include/opus -isysroot /usr/osxcross/bin/../SDK/MacOSX10.10.sdk -mmacosx-version-min=10.10 -m64 -arch x86_64 -DNDEBUG -O3 -Wall"
export LDFLAGS="-L/workdir/cache/macos/usr/lib -isysroot /usr/osxcross/bin/../SDK/MacOSX10.10.sdk -mmacosx-version-min=10.10 -m64 -arch x86_64"

# brew install yasm

# install libsodium, needed for crypto
if ! [ -d libsodium ]; then
  git clone --depth=1 --branch=stable https://github.com/jedisct1/libsodium.git
fi
cd libsodium
git rev-parse HEAD > libsodium.sha
if ! ([ -f "$CACHE_DIR/libsodium.sha" ] && diff "$CACHE_DIR/libsodium.sha" libsodium.sha); then
  ./autogen.sh
  ./configure --prefix="$CACHE_DIR/usr" --host="x86_64-apple-darwin14" --quiet
  # libtool is broken when it comes to spaces in vars, so we have to neuter them
  # I live in backslash escapement hell...
  # This is also why we can't use ../common/*
  find . -type f -exec sed -i 's/libsodium\\\\ 1.0.11/libnacl-str/g' {} +
  make -j8
  make install
  mv libsodium.sha "$CACHE_DIR/libsodium.sha"
fi
cd ..
# rm -rf libsodium


export TARGET_HOST="--host=x86_64-apple-darwin14"
. ./extra/common/build_opus.sh

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch=v1.6.0 https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "$CACHE_DIR/libvpx.sha" ] && diff "$CACHE_DIR/libvpx.sha" libvpx.sha); then
  ./configure --target="x86_64-darwin14-gcc" --prefix=$CACHE_DIR/usr --enable-static --disable-examples --disable-unit-tests --disable-shared
  make -j8
  make install
  mv libvpx.sha "$CACHE_DIR/libvpx.sha"
fi
cd ..
# rm -rf libvpx

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch=master https://github.com/TokTok/c-toxcore.git toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  if [ -d _build ]; then
    rm -rf _build
  fi
  find . -type f -exec sed -i 's/BUILD_TOXAV FALSE/BUILD_TOXAV TRUE/g' {} +
  cmake -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr -DENABLE_STATIC=ON -DENABLE_SHARED=OFF -DCMAKE_SYSTEM_NAME=Darwin -DBUILD_TOXAV=ON
  # mkdir _build
  # autoreconf -fi
  # (cd _build && ../configure --prefix=$CACHE_DIR/usr)
  make -C_build -j8
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
# rm -rf toxcore

ls -la $CACHE_DIR

cmake . -DCMAKE_SYSTEM_NAME=Darwin -DENABLE_ASAN=OFF -DFILTER_AUDIO=OFF -DUTOX_STATIC=OFF -DTOXCORE_STATIC=ON
make
