#/usr/bin/env zsh

# install libsodium, needed for crypto
LIBSODIUM_TAG=${LIBSODIUM_TAG:-"1.0.22-RELEASE"}
if ! [ -d libsodium ]; then
  git clone --depth=1 --branch="$LIBSODIUM_TAG" https://github.com/jedisct1/libsodium.git
fi
cd libsodium
git rev-parse HEAD > libsodium.sha
if ! ([ -f "$CACHE_DIR/libsodium.sha" ] && diff "$CACHE_DIR/libsodium.sha" libsodium.sha); then
  ./autogen.sh
  # Cygwin: inherited SHELLOPTS/errexit breaks autoconf compiler tests.
  case "$(uname -s 2>/dev/null)" in CYGWIN*) set +e ;; esac
  ./configure "$TARGET_HOST" \
              --prefix="$CACHE_DIR/usr" \
              --disable-shared \
              --enable-static
  conf_st=$?
  case "$(uname -s 2>/dev/null)" in CYGWIN*) set -e ;; esac
  if [ "$conf_st" -ne 0 ]; then
    exit "$conf_st"
  fi
  make -j`nproc`
  make install
  mv libsodium.sha "$CACHE_DIR/libsodium.sha"
fi
cd ..
rm -rf libsodium
