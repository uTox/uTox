#!/bin/bash
# Script to cross compile utox for win64 from a linux host with ming32
#
# Expects build dir to be git root with filter_audio and toxcore libs in ../
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
	COMPILE_OPT="-g -O0"
	echo "Building with debugging info"
elif [[ $fast == 1 ]]; then
	COMPILE_OPT="-s -O0"
	echo "Quick build; (without optimizations)"
else
	COMPILE_OPT="-s -Ofast"
fi

# Build filter_audio
AUDIO_FILTERING_BUILD="-D AUDIO_FILTERING -I ../filter_audio/ \
../filter_audio/filter_audio.c ../filter_audio/aec/*.c ../filter_audio/agc/*.c \
../filter_audio/ns/*.c ../filter_audio/other/*.c"

# Remove existing
rm utox.exe 2> /dev/null

# Generate a windows icon
"$WINDOWS_TOOLCHAIN"-windres icons/icon.rc -O coff -o icon.o

# Compile
"$WINDOWS_TOOLCHAIN"-gcc -std=gnu99 $COMPILE_OPT -DAL_LIBTYPE_STATIC -o utox.exe ./*.c     \
./png/png.c $AUDIO_FILTERING_BUILD -I ../toxcore/include/ -I                 \
../openal/include/  ../openal/lib/libOpenAL32.a ../openal/lib/libcommon.a    \
../toxcore/lib/libtoxav.a ../toxcore/lib/libtoxdns.a                         \
../toxcore/lib/libtoxcore.a ../toxcore/lib/libvpx.a ../toxcore/lib/libopus.a \
../toxcore/lib/libsodium.a $MINGW32_LIB_DIR/libwinpthread.a       			 \
-liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 ./icon.o           \
-Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids
