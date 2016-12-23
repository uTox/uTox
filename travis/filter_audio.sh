#!/bin/sh

if ! [ -d filter_audio ]; then
    git clone --depth=1 https://github.com/irungentoo/filter_audio
fi
cd filter_audio
git rev-parse HEAD > filter_audio.sha
if ! ([ -f "$CACHE_DIR/filter_audio.sha" ] && diff "$CACHE_DIR/filter_audio.sha" filter_audio.sha); then
    make
    PREFIX="$HOME/cache/usr/" make install
    mv filter_audio.sha "$CACHE_DIR/filter_audio.sha"
fi
rm -rf filter_audio
