#!/usr/bin/env bash
# Script to cross compile utox for android from linux (and soon windows)
#

set -e
set -x

usage() {
	echo "Cross compile script to build uTox for android from a ~unix environment."
	echo
	echo "Usage: "
	echo "cd [utox_root_directory/]"
	echo "$0"
	echo
	echo "Yup, that's it..."
	return 0
}

for arg in "$@"; do
	case "$arg" in
		*)
			usage
			exit 1
			;;
	esac
done

# read settings from a custom settings file. the defaults don't seem to be very useful.
[ -f settings.android ] && source settings.android

export ALIAS=utox

ANDROID_NDK_HOME=${ANDROID_NDK_HOME-/opt/android-ndk}

if [ ! -d $ANDROID_NDK_HOME ]; then
	echo $ANDROID_NDK_HOME is not a directory >&2
	exit 1
fi

SDK_PATH=${SDK_PATH-/opt/android-sdk}
if [ ! -d $SDK_PATH ]; then
	echo $SDK_PATH is not a directory >&2
	exit 1
fi

TOOLCHAIN_NAME=${TOOLCHAIN_NAME-arm-linux-androideabi-clang3.5}

export ANDROID_NDK_HOME
mkdir -p toolchain
export TOOLCHAIN="$(cd toolchain; pwd)"

"$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh" \
		--ndk-dir="$ANDROID_NDK_HOME" \
		--toolchain="$TOOLCHAIN_NAME" \
		--install-dir=$TOOLCHAIN \
		--platform=android-9

export PATH="$TOOLCHAIN/bin:$PATH"

mkdir -p ./tmp
mkdir -p ./tmp/java
mkdir -p ./tmp/libs
mkdir -p ./tmp/libs/armeabi

ls -la ../openal-arm/lib/ || :
OPENAL_BUILD=${OPENAL_BUILD-'-I../openal-arm/include -L../openal-arm/lib -lOpenSLES'}
NATIVE_AUDIO_BUILD='-DNATIVE_ANDROID_AUDIO -lOpenSLES'

LDFLAGS=${LDFLAGS--L ../toxcore-arm/ -static}
CPPFLAGS=${CPPFLAGS--I../freetype-arm/include/freetype2/ -I../toxcore-arm/include/}
CC=${CC-arm-linux-androideabi-gcc}

SRCDIR=src

${CC} -Wl,--error-unresolved-symbols \
		-Wall -Wextra -s \
		${CPPFLAGS} \
		$SRCDIR/*.c -llog -landroid -lEGL -lGLESv2 $OPENAL_BUILD \
		${LDFLAGS} \
		-lopenal \
		-ltoxcore                                \
		-ltoxdns \
		-ltoxav \
		-lsodium \
		-lopus \
		-lvpx \
		-lfreetype \
		-lm -lz -ldl -shared -o ./tmp/libs/armeabi/libuTox.so

AAPT={$SDK_PATH/build-tools/21.1.2/aapt-aapt}
$AAPT package -f -M ./android/AndroidManifest.xml -S ./android/res \
		-I $SDK_PATH/platforms/android-21/android.jar -F ./tmp/tmp1.apk -J ./tmp/java

javac -d ./tmp/java ./tmp/java/R.java

$SDK_PATH/build-tools/21.1.2/dx --dex --output=./tmp/classes.dex ./tmp/java

java -classpath $SDK_PATH/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain ./tmp/tmp2.apk \
    	-u -z ./tmp/tmp1.apk -f ./tmp/classes.dex -nf ./tmp/libs

jarsigner -sigalg SHA1withRSA -digestalg SHA1 -keystore ./debug.keystore -storepass $PASSWORD ./tmp/tmp2.apk $ALIAS

mv tmp/tmp2.apk ./utox.apk

rm -r tmp toolchain

