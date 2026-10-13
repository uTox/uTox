# Build and install c-toxcore into $CACHE_DIR/usr (skip if cache sha matches).
#
# Optional env:
#   TOXCORE_CMAKE_ARGS — extra cmake flags (word-split; use only -D... tokens)

if ! [ -d toxcore ]; then
  git clone --depth=1 --recurse-submodules --branch="$TOXCORE_REPO_BRANCH" "$TOXCORE_REPO_URI" toxcore
fi
cd toxcore
git submodule update --init --recursive
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  # ${TOXCORE_CMAKE_ARGS:-} is intentionally unquoted (extra -D flags).
  # shellcheck disable=SC2086
  cmake -DBOOTSTRAP_DAEMON=OFF \
        -DDHT_BOOTSTRAP=OFF \
        -DCMAKE_INSTALL_PREFIX:PATH="$CACHE_DIR/usr" \
        ${TOXCORE_CMAKE_ARGS:-} \
        -B_build -H.
  if command -v nproc >/dev/null 2>&1; then
    jobs=$(nproc)
  elif command -v sysctl >/dev/null 2>&1; then
    jobs=$(sysctl -n hw.ncpu)
  else
    jobs=2
  fi
  make -C_build -j"$jobs"
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore
