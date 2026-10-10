#/usr/bin/env zsh

LIBVPX_TAG=${LIBVPX_TAG:-"v1.16.0"}

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch="$LIBVPX_TAG" https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "${CACHE_DIR}/libvpx.sha" ] && diff "${CACHE_DIR}/libvpx.sha" libvpx.sha); then
  VPX_CFLAGS=""
  case "$TARGET_TRGT" in
    *win*) VPX_CFLAGS="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -fno-asynchronous-unwind-tables" ;;
  esac
  case "$(uname -s 2>/dev/null)" in CYGWIN*) set +e ;; esac
  CFLAGS="${CFLAGS:-} ${VPX_CFLAGS}" \
  CROSS="${CROSS}" \
  ./configure "$TARGET_TRGT" \
              --prefix="${CACHE_DIR}/usr" \
              --enable-static \
              --disable-examples \
              --disable-unit-tests \
              --disable-tools \
              --disable-shared
  conf_st=$?
  case "$(uname -s 2>/dev/null)" in CYGWIN*) set -e ;; esac
  if [ "$conf_st" -ne 0 ]; then
    exit "$conf_st"
  fi
  make -j8
  make install
  mv libvpx.sha "${CACHE_DIR}/libvpx.sha"
fi
cd ..
rm -rf libvpx
