#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

export TARGET_HOST="--host=x86_64-w64-mingw32"
export TARGET_TRGT="--target=x86_64-win32-gcc"
export CROSS="x86_64-w64-mingw32-"

. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh
. ./extra/common/build_vpx.sh

# install toxcore
git clone --depth=1 --branch=$TOXCORE_REPO_BRANCH $TOXCORE_REPO_URI toxcore
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  mkdir _build
  cmake -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -B_build \
        -H. \
        -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr \
        -DENABLE_SHARED=OFF \
        -DENABLE_STATIC=ON
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
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH $CACHE_DIR )
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" > ./Toolchain-x86_64-w64-mingw32.cmake
  cmake ..  -DCMAKE_TOOLCHAIN_FILE=./Toolchain-x86_64-w64-mingw32.cmake \
            -DCMAKE_PREFIX_PATH="$CACHE_DIR/usr" \
            -DCMAKE_INSTALL_PREFIX="$CACHE_DIR/usr" \
            -DLIBTYPE="STATIC" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DDSOUND_INCLUDE_DIR=/usr/x86_64-w64-mingw32/include \
            -DDSOUND_LIBRARY=/usr/x86_64-w64-mingw32/lib/libdsound.a
  make
  make install
  cd ..
  mv openal.sha "$CACHE_DIR/openal.sha"
fi
cd ..
rm -rf openal

export CC=x86_64-w64-mingw32-gcc
. ./extra/common/filter_audio.sh
x86_64-w64-mingw32-ranlib $CACHE_DIR/usr/lib/libfilteraudio.a
unset CC

cp $CACHE_DIR/usr/lib/libOpenAL32.a $CACHE_DIR/usr/lib/libopenal.a || true
if [ ! -f "$CACHE_DIR/usr/lib/libshell32.a" ] then;
    curl https://cmdline.org/travis/64/shell32.a > "$CACHE_DIR/usr/lib/libshell32.a"
fi


