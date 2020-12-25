#/usr/bin/env zsh

# install libsodium, needed for crypto
if ! [ -d libsodium ]; then
  git clone --depth=1 --branch=stable https://github.com/jedisct1/libsodium.git
fi
cd libsodium
git rev-parse HEAD > libsodium.sha
if ! ([ -f "$CACHE_DIR/libsodium.sha" ] && diff "$CACHE_DIR/libsodium.sha" libsodium.sha); then
  ./autogen.sh
  ./configure "$TARGET_HOST" \
              --prefix="$CACHE_DIR/usr"
  make -j`nproc`
  make install
  mv libsodium.sha "$CACHE_DIR/libsodium.sha"
fi
cd ..
rm -rf libsodium
