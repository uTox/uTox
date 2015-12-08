#!/bin/bash
# Script to cross compile utox for win64 from a linux host with ming32
#
# Expects build dir to be git root with filter_audio and toxcore libs in ./lib/
#
# TODO: Add support to -t path/to/libtoxcore; -l path/to/openal; -f path/to/filter_audio

usage() {
    echo "Cross compile script to build windows uTox from a ~unix enviroment."
    echo
    echo "Usage: $0 [32|64] [win|unix] [fast|debug]"
    return 0
}

for arg in "$@"; do
    case "$arg" in
        32 | '-32')
            arch=32
            ;;
        64 | '-64')
            arch=64
            ;;
        'unix' | '-u')
            host=0
            ;;
        'win' | '-w')
            host=1
            ;;
        'fast' | '-f')
            fast=1
            ;;
        'debug' | '-d')
            debug=1
            ;;
        'legacy' | '-l')
            legacy=1
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

# Build Architecture
if [[ $arch == 32 ]]; then
    WINDOWS_TOOLCHAIN=i686-w64-mingw32
    echo "Building 32bit version of uTox"
elif [[ $arch == 64 ]]; then
    WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
    echo "Building 64bit version of uTox"
else
    WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
    echo "Defaulting to 64bit"
fi

# Build Environment
if [[ $host == 0 ]]; then
    MINGW32_LIB_DIR="/usr/$WINDOWS_TOOLCHAIN/lib"
    echo "Building from a native Unix environment"
elif [[ $host == 1 ]]; then
    MINGW32_LIB_DIR="/usr/$WINDOWS_TOOLCHAIN/sys-root/mingw/lib"
    echo "Building from a ~Unix environment within windows"
else
    MINGW32_LIB_DIR="/usr/$WINDOWS_TOOLCHAIN/sys-root/mingw/lib"
    echo "Defaulting to a ~Unix environment within windows"
fi

# Other options
if [[ $debug  == 1 ]]; then
    COMP_OPTs="-g -O0"
    echo "Building with debugging info"
elif [[ $fast == 1 ]]; then
    COMP_OPTs="-s -O0"
    echo "Quick build (without optimizations)"
else
    COMP_OPTs="-s -Ofast"
fi

if [[ $legacy == 1 ]]; then
    COMP_OPTs+=" -D __WIN_LEGACY=1"
    echo "Compiling for windows XP"
fi

LIBTOXCORE="../toxcore/build/.libs/"
LIBNACL="./lib/libsodium"
LIBVPX="./lib/vpx"
LIBOPUS="./lib/opus"
LIBOPENAL="./lib/openal"

GIT_V=`git describe --abbrev=8 --dirty --always --tags`
echo -n "Git version: "
git describe --abbrev=8 --dirty --always --tags

# Build filter_audio
AUDIO_FILTERING_BUILD="-DAUDIO_FILTERING -I ./lib/filter_audio/ ./lib/filter_audio/filter_audio.c \
./lib/filter_audio/aec/*.c ./lib/filter_audio/agc/*.c ./lib/filter_audio/ns/*.c ./lib/filter_audio/other/*.c \
./lib/filter_audio/vad/*.c ./lib/filter_audio/zam/*.c"

# Remove existing
rm utox.exe 2> /dev/null

# Generate a windows icon
"$WINDOWS_TOOLCHAIN"-windres icons/icon.rc -O coff -o icon.o

# Compile
"$WINDOWS_TOOLCHAIN"-gcc -o utox.exe  $COMP_OPTs                         \
    -I /usr/local/include/                                               \
    -DGIT_VERSION=\"$GIT_V\" -DAL_LIBTYPE_STATIC                         \
    ./*.c ./png/png.c ./icon.o                                           \
    $LIBTOXCORE/libtoxcore.a                                             \
    $LIBTOXCORE/libtoxav.a                                               \
    $LIBTOXCORE/libtoxdns.a                                              \
    $LIBTOXCORE/libtoxencryptsave.a                                      \
    $LIBNACL/lib/libsodium.a             -I $LIBNACL/include/            \
    $LIBVPX/lib/libvpx.a                 -I $LIBVPX/include/             \
    $LIBOPUS/lib/libopus.a               -I $LIBOPUS/include/            \
    $LIBOPENAL/lib/libOpenAL32.a         -I $LIBOPENAL/include/          \
    $MINGW32_LIB_DIR/libwinpthread.a                                     \
    $AUDIO_FILTERING_BUILD                                               \
    -std=gnu99 -liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 \
    -Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids
