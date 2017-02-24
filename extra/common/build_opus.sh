#/usr/bin/env zsh

# install libopus, needed for audio encoding/decoding
if ! [ -f $CACHE_DIR/usr/lib/pkgconfig/opus.pc ]; then
  curl http://downloads.xiph.org/releases/opus/opus-1.1.4.tar.gz -o opus.tar.gz
  tar xzf opus.tar.gz
  cd opus-1.1.4
  ./configure --prefix="$CACHE_DIR/usr" $TARGET_HOST
  make -j`nproc`
  make install
  cd ..
  rm -rf opus**
fi
