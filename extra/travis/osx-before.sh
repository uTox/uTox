#!/bin/sh

set -eux

. ./extra/travis/env.sh

brew install yasm

. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch=v1.6.0 https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "$CACHE_DIR/libvpx.sha" ] && diff "$CACHE_DIR/libvpx.sha" libvpx.sha); then
  ./configure --prefix="$CACHE_DIR/usr" --enable-shared
  make -j`sysctl -n hw.ncpu`
  make install
  mv libvpx.sha "$CACHE_DIR/libvpx.sha"
fi
cd ..
rm -rf libvpx

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch="$TOXCORE_REPO_BRANCH" "$TOXCORE_REPO_URI" toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH="$CACHE_DIR/usr"
  make -C_build -j`sysctl -n hw.ncpu`
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore
