#!/bin/sh

set -eux

. ./extra/travis/env.sh

FILTER_AUDIO_TAG=${FILTER_AUDIO_TAG:-"v0.0.1"}

if ! [ -d filter_audio ]; then
    git clone --depth=1 --branch="$FILTER_AUDIO_TAG" https://github.com/irungentoo/filter_audio
fi
cd filter_audio
git rev-parse HEAD > filter_audio.sha
if ! ([ -f "$CACHE_DIR/filter_audio.sha" ] && diff "$CACHE_DIR/filter_audio.sha" filter_audio.sha); then
    make
    if [ ! -f libfilteraudio.a ]; then
        AR_BIN="${AR:-ar}"
        case "$CC" in
            *mingw*) AR_BIN="${CC%-gcc}-ar" ;;
        esac
        "$AR_BIN" rcs libfilteraudio.a $(find . -name '*.o')
    fi
    PREFIX="${CACHE_DIR}/usr/" make install
    if [ -f libfilteraudio.a ] && [ ! -f "${CACHE_DIR}/usr/lib/libfilteraudio.a" ]; then
        install -m 0644 libfilteraudio.a "${CACHE_DIR}/usr/lib/libfilteraudio.a"
    fi
    mv filter_audio.sha "$CACHE_DIR/filter_audio.sha"
fi
cd ..
rm -rf filter_audio
