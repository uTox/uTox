#!/bin/sh
set -eux

. ./extra/gitlab/env.sh

export TARGET_HOST="--host=i686-w64-mingw32"

. ./extra/common/build_nacl.sh
. ./extra/common/build_opus.sh

# install libvpx, needed for video encoding/decoding
if ! [ -d libvpx ]; then
  git clone --depth=1 --branch=v1.6.0 https://chromium.googlesource.com/webm/libvpx
fi
cd libvpx
git rev-parse HEAD > libvpx.sha
if ! ([ -f "$CACHE_DIR/libvpx.sha" ] && diff "$CACHE_DIR/libvpx.sha" libvpx.sha); then
  CROSS=i686-w64-mingw32- ./configure --target=x86-win32-gcc --prefix=$CACHE_DIR/usr --disable-examples --disable-unit-tests --disable-shared --enable-static
  make -j`nproc`
  make install
  mv libvpx.sha "$CACHE_DIR/libvpx.sha"
fi
cd ..
rm -rf libvpx

# install toxcore
if ! [ -d toxcore ]; then
  git clone --depth=1 --branch=master https://github.com/TokTok/c-toxcore.git toxcore
fi
cd toxcore
git rev-parse HEAD > toxcore.sha
if ! ([ -f "$CACHE_DIR/toxcore.sha" ] && diff "$CACHE_DIR/toxcore.sha" toxcore.sha); then
  if [ -f "./CMakeFiles.txt" ]; then
    cmake -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -B_build -H. -DCMAKE_INSTALL_PREFIX:PATH=$CACHE_DIR/usr -DENABLE_SHARED=0
  else
    mkdir _build
    autoreconf -fi
    (cd _build && ../configure --host=i686-w64-mingw32 --prefix=$CACHE_DIR/usr)
  fi
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

export CC=i686-w64-mingw32-gcc
. ./extra/gitlab/filter_audio.sh
i686-w64-mingw32-ranlib $CACHE_DIR/usr/lib/libfilteraudio.a
unset CC

cp $CACHE_DIR/usr/lib/libOpenAL32.a $CACHE_DIR/usr/lib/libopenal.a || true
# sudo curl https://cmdline.org/travis/32/shell32.a > $CACHE_DIR/usr/lib/libshell32.a
