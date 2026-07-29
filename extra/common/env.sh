#!/bin/sh

export CTEST_OUTPUT_ON_FAILURE=1

export CACHE_DIR=${CACHE_DIR=""}

export TOXCORE_REPO=${TOXCORE_REPO:-"TokTok/c-toxcore"}
export TOXCORE_REPO_URI="https://github.com/TokTok/c-toxcore.git"
export TOXCORE_REPO_BRANCH=${TOXCORE_REPO_BRANCH:-"v0.2.23"}

export LIBSODIUM_TAG=${LIBSODIUM_TAG:-"1.0.22-RELEASE"}
export OPUS_VERSION=${OPUS_VERSION:-"1.6.1"}
export OPUS_SHA256=${OPUS_SHA256:-"6ffcb593207be92584df15b32466ed64bbec99109f007c82205f0194572411a1"}
export LIBVPX_TAG=${LIBVPX_TAG:-"v1.16.0"}
export OPENAL_REPO_URI=${OPENAL_REPO_URI:-"https://github.com/uTox/openal-soft-tox.git"}
export OPENAL_REF=${OPENAL_REF:-"master"}
export FILTER_AUDIO_TAG=${FILTER_AUDIO_TAG:-"v0.0.1"}

export CFLAGS="-I${CACHE_DIR}/usr/include -I${CACHE_DIR}/usr/include/opus"
export LDFLAGS="-L${CACHE_DIR}/usr/lib"

export LD_LIBRARY_PATH="${CACHE_DIR}/usr/lib:/usr/lib"

export PKG_CONFIG_PATH="${CACHE_DIR}/usr/lib/pkgconfig"

# Cross compilation default targets.
export TARGET_HOST=${TARGET_HOST:-""}
export TARGET_TRGT=${TARGET_TRGT:-""}

export MAKEFLAGS="-j8"

# Cygwin + Windows Git: force LF checkouts (CRLF breaks Autotools).
case "$(uname -s 2>/dev/null)" in
  CYGWIN*|MSYS*|MINGW*)
    export GIT_CONFIG_COUNT=2
    export GIT_CONFIG_KEY_0=core.autocrlf
    export GIT_CONFIG_VALUE_0=false
    export GIT_CONFIG_KEY_1=core.eol
    export GIT_CONFIG_VALUE_1=lf
    ;;
esac
