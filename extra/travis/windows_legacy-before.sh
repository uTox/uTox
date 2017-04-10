#!/bin/sh
set -eux

. ./extra/travis/env.sh

export TARGET_HOST="--host=i686-w64-mingw32"

# Travis is very odd...
export NACL_FLAGS="${NACL_FLAGS} --disable-ssp"


. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh
. ./extra/common/build_vpx.sh

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch="$TOXCORE_REPO_BRANCH" "$TOXCORE_REPO_URI" toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH="$HOME/cache/usr" -DENABLE_SHARED=0
  make -C_build -j`nproc`
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore

if ! [ -d openal ]; then
  git clone --depth=1 https://github.com/irungentoo/openal-soft-tox.git openal
fi
cd openal
git rev-parse HEAD > openal.sha
if ! ([ -f "$CACHE_DIR/openal.sha" ] && diff "$CACHE_DIR/openal.sha" openal.sha ); then
  mkdir -p build
  cd build
  echo "
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH $CACHE_DIR )
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" > ./Toolchain-i686-w64-mingw32.cmake
  cmake ..  -DCMAKE_TOOLCHAIN_FILE=./Toolchain-i686-w64-mingw32.cmake \
            -DCMAKE_PREFIX_PATH="$CACHE_DIR/usr" \
            -DCMAKE_INSTALL_PREFIX="$CACHE_DIR/usr" \
            -DLIBTYPE="STATIC" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DDSOUND_INCLUDE_DIR=/usr/i686-w64-mingw32/include \
            -DDSOUND_LIBRARY=/usr/i686-w64-mingw32/lib/libdsound.a
  make
  make install
  cd ..
  mv openal.sha "$CACHE_DIR/openal.sha"
fi
cd ..
rm -rf openal

cp "$CACHE_DIR/usr/lib/libOpenAL32.a" "$CACHE_DIR/usr/lib/libopenal.a" || true
# sudo curl https://cmdline.org/travis/32/shell32.a > $CACHE_DIR/usr/lib/libshell32.a
