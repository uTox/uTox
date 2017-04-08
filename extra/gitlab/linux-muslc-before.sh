#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

export TARGET_TRGT="--enable-pic "
export CFLAGS="${CFLAGS} -fPIC"
. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh
. ./extra/common/build_vpx.sh

# install toxcore
rm -rf toxcore
git clone --depth=1 --branch=$TOXCORE_REPO_BRANCH $TOXCORE_REPO_URI toxcore
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -B_build -H. \
        -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr \
        -DENABLE_STATIC=ON \
        -DENABLE_SHARED=OFF \
        -DBUILD_TOXAV=ON \
        -DPKG_CONFIG_EXECUTABLE=/usr/bin/pkg-config

  make -C_build -j`nproc` || make VERBOSE=1
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore
