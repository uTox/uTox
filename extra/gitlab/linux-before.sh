#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch=v1.6.0 https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "$CACHE_DIR/libvpx.sha" ] && diff "$CACHE_DIR/libvpx.sha" libvpx.sha); then
  ./configure --prefix=$CACHE_DIR/usr \
              --enable-shared \
              --disable-unit-tests \
              --disable-install-docs \
              --disable-examples
  make -j`nproc`
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
  if [ -f CMakeFiles.txt ]; then
    cmake -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr
  else
    mkdir _build
    autoreconf -fi
    (cd _build && ../configure --prefix=$CACHE_DIR/usr)
  fi
  make -C_build -j`nproc`
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore
