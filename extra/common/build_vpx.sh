#/usr/bin/env zsh

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch=v1.6.0 https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "${CACHE_DIR}/libvpx.sha" ] && diff "${CACHE_DIR}/libvpx.sha" libvpx.sha); then
  ./configure "$TARGET_TRGT" \
              --prefix="${CACHE_DIR}/usr" \
              --enable-static \
              --disable-examples \
              --disable-unit-tests \
              --disable-shared
  make -j8
  make install
  mv libvpx.sha "${CACHE_DIR}/libvpx.sha"
fi
cd ..
rm -rf libvpx
