#!/bin/sh

set -eux

. ./extra/travis/env.sh

brew install yasm

. ./extra/common/build_nacl.sh

# install libopus, needed for audio encoding/decoding
if ! [ -f $CACHE_DIR/usr/lib/pkgconfig/opus.pc ]; then
  curl http://downloads.xiph.org/releases/opus/opus-1.1.4.tar.gz -o opus.tar.gz
  tar xzf opus.tar.gz
  cd opus-1.1.4
  ./configure --prefix=$CACHE_DIR/usr
  make -j`systctl -n hw.ncpu`
  make install
  cd ..
  rm -rf opus**
fi

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch=v1.6.0 https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "$CACHE_DIR/libvpx.sha" ] && diff "$CACHE_DIR/libvpx.sha" libvpx.sha); then
  ./configure --prefix=$CACHE_DIR/usr --enable-shared
  make -j`sysctl -n hw.ncpu`
  make install
  mv libvpx.sha "$CACHE_DIR/libvpx.sha"
fi
cd ..
rm -rf libvpx

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch=master https://github.com/$TOXCORE_REPO.git toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr
  make -C_build -j`sysctl -n hw.ncpu`
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore
