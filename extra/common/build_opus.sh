#/usr/bin/env zsh

OPUS_VERSION=${OPUS_VERSION:-"1.6.1"}

# install libopus, needed for audio encoding/decoding
if ! [ -f "$CACHE_DIR/usr/lib/pkgconfig/opus.pc" ]; then
  opus_archive="opus.tar.gz"
  curl -L "https://downloads.xiph.org/releases/opus/opus-${OPUS_VERSION}.tar.gz" -o "$opus_archive"
  if [ -z "${OPUS_SHA256:-}" ]; then
    echo "OPUS_SHA256 is not set; refusing to build opus-${OPUS_VERSION} without checksum" >&2
    rm -f "$opus_archive"
    exit 1
  fi
  actual_sha256=$(sha256sum "$opus_archive" | awk '{print $1}')
  if [ "$actual_sha256" != "$OPUS_SHA256" ]; then
    echo "opus-${OPUS_VERSION}.tar.gz SHA256 mismatch" >&2
    echo "  expected: $OPUS_SHA256" >&2
    echo "  actual:   $actual_sha256" >&2
    rm -f "$opus_archive"
    exit 1
  fi
  tar xzf "$opus_archive"
  cd opus-${OPUS_VERSION}
  # MinGW static link: avoid fortified memcpy and SEH/unwind ICEs.
  OPUS_CFLAGS=""
  case "$TARGET_HOST" in
    *mingw*) OPUS_CFLAGS="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fno-asynchronous-unwind-tables" ;;
  esac
  case "$(uname -s 2>/dev/null)" in CYGWIN*) set +e ;; esac
  CFLAGS="${CFLAGS:-} ${OPUS_CFLAGS}" \
  ./configure "$TARGET_HOST" \
              --prefix="$CACHE_DIR/usr" \
              --disable-extra-programs \
              --disable-doc \
              --disable-shared \
              --enable-static
  conf_st=$?
  case "$(uname -s 2>/dev/null)" in CYGWIN*) set -e ;; esac
  if [ "$conf_st" -ne 0 ]; then
    exit "$conf_st"
  fi
  make -j`nproc`
  make install
  cd ..
  rm -rf opus-${OPUS_VERSION} "$opus_archive"
else
  echo "Have Opus"
  ls -la "${CACHE_DIR}/usr/lib/"
  ls -la "${CACHE_DIR}/usr/lib/pkgconfig/"
  ls -la "${CACHE_DIR}/usr/include/"
fi
