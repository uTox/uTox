#!/bin/sh

set -eux

. ./extra/ci/env.sh

export HOMEBREW_NO_INSTALL_CLEANUP=1
# GitHub macOS runners pre-tap aws/tap (and sometimes azure/bicep); Homebrew 6+
# warns on every brew command until they are trusted or untapped. We don't use them.
brew untap aws/tap 2>/dev/null || true
brew untap azure/bicep 2>/dev/null || true
brew install check
brew install libsodium
brew install libvpx
brew install opus

TOXCORE_CMAKE_ARGS="-DENABLE_SHARED=OFF"
export TOXCORE_CMAKE_ARGS
. ./extra/ci/common/build_toxcore.sh
