#!/bin/sh

set -eux

. ./extra/travis/env.sh

export HOMEBREW_NO_INSTALL_CLEANUP=1
brew install check
brew install libsodium
brew install libvpx
brew install opus

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch="$TOXCORE_REPO_BRANCH" "$TOXCORE_REPO_URI" toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -DENABLE_SHARED=OFF -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH="$CACHE_DIR/usr"
  make -C_build -j`sysctl -n hw.ncpu`
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore
