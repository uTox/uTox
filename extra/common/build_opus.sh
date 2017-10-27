#/usr/bin/env zsh

OPUS_VERSION="1.2.1"

# install libopus, needed for audio encoding/decoding
if ! [ -f "$CACHE_DIR/usr/lib/pkgconfig/opus.pc" ]; then
  curl https://archive.mozilla.org/pub/opus/opus-${OPUS_VERSION}.tar.gz -o opus.tar.gz
  tar xzf opus.tar.gz
  cd opus-${OPUS_VERSION}
  ./configure "$TARGET_HOST" \
              --prefix="$CACHE_DIR/usr" \
              --disable-extra-programs \
              --disable-doc
  make -j`nproc`
  make install
  cd ..
  rm -rf opus**
else
  echo "Have Opus"
  ls -la "${CACHE_DIR}/usr/lib/"
  ls -la "${CACHE_DIR}/usr/lib/pkgconfig/"
  ls -la "${CACHE_DIR}/usr/include/"
fi
