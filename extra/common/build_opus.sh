#/usr/bin/env zsh

# install libopus, needed for audio encoding/decoding
if ! [ -f "$CACHE_DIR/usr/lib/pkgconfig/opus.pc" ]; then
  curl http://downloads.xiph.org/releases/opus/opus-1.1.4.tar.gz -o opus.tar.gz
  tar xzf opus.tar.gz
  cd opus-1.1.4
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
