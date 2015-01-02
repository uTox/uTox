#!/bin/bash
# Script to cross compile utox for win64 from a linux host with ming32
#
# Expects build dir to be git root with filter_audio and toxcore libs in ../

if [[ $1 == 32 ]]; then
	WINDOWS_TOOLCHAIN=i686-w64-mingw32
	echo "Building 32bit version of utox"
elif [[ $1 == 64 ]]; then
	WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
	echo "Building 64xbit version of utox"
elif [[ $1 != '' ]]; then
	echo "First argument must either 32 or 64"
	exit 1
else
	WINDOWS_TOOLCHAIN=x86_64-w64-mingw32
	echo "Defaulting to 64bit"
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
"$WINDOWS_TOOLCHAIN"-gcc -s -Ofast -DAL_LIBTYPE_STATIC -o utox.exe ./*.c     \
./png/png.c $AUDIO_FILTERING_BUILD -I ../toxcore/include/ -I                 \
../openal/include/  ../openal/lib/libOpenAL32.a ../openal/lib/libcommon.a    \
../toxcore/lib/libtoxav.a ../toxcore/lib/libtoxdns.a                         \
../toxcore/lib/libtoxcore.a ../toxcore/lib/libvpx.a ../toxcore/lib/libopus.a \
../toxcore/lib/libsodium.a /usr/$WINDOWS_TOOLCHAIN/lib/libwinpthread.a       \
-liphlpapi -lws2_32 -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 ./icon.o           \
-Wl,-subsystem,windows -lwinmm -lole32 -loleaut32 -lstrmiids
