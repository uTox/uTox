#!/bin/sh
set -eux

. ./extra/travis/env.sh

export TARGET_HOST="--host=x86_64-w64-mingw32"
export TARGET_TRGT="--target=x86_64-win64-gcc"
export CROSS="x86_64-w64-mingw32-"

. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh
. ./extra/common/build_vpx.sh

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --recurse-submodules --branch="$TOXCORE_REPO_BRANCH" "$TOXCORE_REPO_URI" toxcore
fi
cd toxcore
git submodule update --init --recursive
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_FIND_ROOT_PATH="$CACHE_DIR/usr" \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_INSTALL_PREFIX:PATH="$CACHE_DIR/usr" \
        -DCMAKE_PREFIX_PATH="$CACHE_DIR/usr" \
        -DENABLE_SHARED=OFF \
        -DENABLE_STATIC=ON \
        -DBUILD_TOXAV=ON \
        -DMUST_BUILD_TOXAV=ON \
        -DBOOTSTRAP_DAEMON=OFF \
        -DDHT_BOOTSTRAP=OFF \
        -DBUILD_MISC_TESTS=OFF \
        -DAUTOTEST=OFF \
        -DUNITTEST=OFF \
        -B_build -H.
  make -C_build -j$(nproc)
  make -C_build install
  mv toxcore.sha "$CACHE_DIR/toxcore.sha"
fi
cd ..
rm -rf toxcore

if ! [ -d openal ]; then
  git clone --depth=1 --branch="$OPENAL_REF" "$OPENAL_REPO_URI" openal
fi
cd openal
git rev-parse HEAD > openal.sha
if ! ([ -f "$CACHE_DIR/openal.sha" ] && diff "$CACHE_DIR/openal.sha" openal.sha ); then
  mkdir -p build
  cd build
  echo "
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH $CACHE_DIR/usr /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" > ./Toolchain-x86_64-w64-mingw32.cmake
  cmake ..  -DCMAKE_TOOLCHAIN_FILE=./Toolchain-x86_64-w64-mingw32.cmake \
            -DCMAKE_PREFIX_PATH="$CACHE_DIR/usr" \
            -DCMAKE_INSTALL_PREFIX="$CACHE_DIR/usr" \
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
            -DCMAKE_C_FLAGS="-Wno-incompatible-pointer-types" \
            -DLIBTYPE="STATIC" \
            -DCMAKE_BUILD_TYPE=Release \
            -DALSOFT_EXAMPLES=OFF \
            -DALSOFT_UTILS=OFF \
            -DDSOUND_INCLUDE_DIR=/usr/x86_64-w64-mingw32/include \
            -DDSOUND_LIBRARY=/usr/x86_64-w64-mingw32/lib/libdsound.a
  make
  make install
  cd ..
  mv openal.sha "$CACHE_DIR/openal.sha"
fi
cd ..
rm -rf openal

# uTox includes <OpenAL/...>; OpenAL Soft installs headers under AL/
if [ -d "$CACHE_DIR/usr/include/AL" ] && [ ! -e "$CACHE_DIR/usr/include/OpenAL" ]; then
  ln -s AL "$CACHE_DIR/usr/include/OpenAL"
fi

export CC=x86_64-w64-mingw32-gcc
. ./extra/common/filter_audio.sh
x86_64-w64-mingw32-ranlib "$CACHE_DIR/usr/lib/libfilteraudio.a"
unset CC

cp "$CACHE_DIR/usr/lib/libOpenAL32.a" "$CACHE_DIR/usr/lib/libopenal.a" || true
if [ ! -f "$CACHE_DIR/usr/lib/libOpenAL32.a" ] && [ -f "$CACHE_DIR/usr/lib/libopenal.a" ]; then
  cp "$CACHE_DIR/usr/lib/libopenal.a" "$CACHE_DIR/usr/lib/libOpenAL32.a"
fi
