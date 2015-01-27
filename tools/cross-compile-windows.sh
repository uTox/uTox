#!/bin/bash
# Script to cross compile utox for win64 from a linux host with ming32
#
# Expects build dir to be git root with filter_audio and toxcore libs in ../
#
# TODO: Add support to -t path/to/libtoxcore; -l path/to/openal; -f path/to/filter_audio

usage() {
	echo "Cross compile script to build windows uTox from a ~unix enviroment."
	echo 
	echo "Usage: $0 [32|64] [win|unix]"
	return 0
}

for arg in "$@"; do
	case "$arg" in
		32)
			WINDOWS_TOOLCHAIN=i686-w64-mingw32
			echo "Building 32bit version of utox"
			;;
		64)
			WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
			echo "Building 64bit version of utox"
			;;
		"unix")
			if [[ -z $WINDOWS_TOOLCHAIN ]]; then
				echo "Bit version must be supplied BEFORE $arg"
				exit 1
			else
				MINGW32_LIB_DIR="/usr/$WINDOWS_TOOLCHAIN/lib"
				echo "Building from a native unix enviroment"
			fi
			;;
		"win")
			if [[ -z $WINDOWS_TOOLCHAIN ]]; then
				echo "Bit version must be supplied BEFORE $arg"
				exit 1
			else
				MINGW32_LIB_DIR="/usr/$WINDOWS_TOOLCHAIN/sys-root/mingw/lib"
				echo "Building from a ~unix enviroment within windows"
			fi
			;;
		*)
			;;
	esac
done

if [[ -z $WINDOWS_TOOLCHAIN && -z $MINGW32_LIB_DIR ]]; then
	WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
	MINGW32_LIB_DIR="/usr/$WINDOWS_TOOLCHAIN/lib"
	echo "Defaulting to 64bit, and native unix enviroment"
elif [[ -z $WINDOWS_TOOLCHAIN ]]; then
	WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
	echo "Defaulting to 64bit"
elif [[ -z $MINGW32_LIB_DIR ]]; then
	MINGW32_LIB_DIR=$MINGW32_LIB_DIR_UNIX
	echo "Defaulting to native unix"
fi


# Build filter_audio
AUDIO_FILTERING_BUILD="-D AUDIO_FILTERING -I ../filter_audio/ \
../filter_audio/filter_audio.c ../filter_audio/aec/*.c ../filter_audio/agc/*.c \
../filter_audio/ns/*.c ../filter_audio/other/*.c"

# Remove existing
rm utox.exe

# Generate a windows icon
"$WINDOWS_TOOLCHAIN"-windres icons/icon.rc -O coff -o icon.o

# Compile
"$WINDOWS_TOOLCHAIN"-gcc -std=gnu99 -s -Ofast -DAL_LIBTYPE_STATIC -o utox.exe ./*.c     \
./png/png.c $AUDIO_FILTERING_BUILD -I ../toxcore/include/ -I                 \
../openal/include/  ../openal/lib/libOpenAL32.a ../openal/lib/libcommon.a    \
../toxcore/lib/libtoxav.a ../toxcore/lib/libtoxdns.a                         \
../toxcore/lib/libtoxcore.a ../toxcore/lib/libvpx.a ../toxcore/lib/libopus.a \
../toxcore/lib/libsodium.a $MINGW32_LIB_DIR/libwinpthread.a       			 \
-liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 ./icon.o           \
-Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids
