#!/usr/bin/env bash
set -ex

mkdir -p ./build_android/{lib/armeabi,java}

# read settings from a custom settings file.
[ -f settings.android ] && source settings.android

# ./lib/arm/lib is the compilation of all the required dependencies you can scrape from build.tox.chat needed to
# cross compile uTox to Android, you might choose to store them elsewhere.
LDFLAGS=${LDFLAGS--L./libs/android/lib/}

DEV_VERSION="25.0.0"
SDK_VERSION="android-23"
NDK_VERSION="android-12"

KEYSTORE=${KEYSTORE-~/.android/utox.keystore}

# Standard dev kit locations on posix
ANDROID_NDK_HOME=${ANDROID_NDK_HOME-/opt/android-ndk}
ANDROID_SDK_HOME=${ANDROID_SDK_HOME-/opt/android-sdk}
SYSROOT=${SYSROOT-${ANDROID_NDK_HOME}/platforms/${NDK_VERSION}/arch-arm}
TOOLCHAIN=${TOOLCHAIN-./toolchain}
AAPT=${AAPT-$ANDROID_SDK_HOME/build-tools/${DEV_VERSION}/aapt}

#
## Build the local toolchain. (This section can also be used to compile toxcore.)
#
[ -d ${TOOLCHAIN} ] || "$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh" \
        --toolchain="arm-linux-androideabi-clang" \
        --install-dir=$TOOLCHAIN/ \
        --platform=${NDK_VERSION}

#
## All the libraries uTox is going to nee to link against
#
TOX_LIBS=${TOX_LIBS-\
    ./libs/android/lib/libtoxencryptsave.a \
    ./libs/android/lib/libtoxdns.a \
    ./libs/android/lib/libtoxav.a \
    ./libs/android/lib/libtoxcore.a \
    ./libs/android/lib/libtoxgroup.a \
    ./libs/android/lib/libtoxmessenger.a \
    ./libs/android/lib/libtoxfriends.a \
    ./libs/android/lib/libtoxdht.a \
    ./libs/android/lib/libtoxnetcrypto.a \
    ./libs/android/lib/libtoxnetwork.a \
    ./libs/android/lib/libtoxcrypto.a \
    }
# Ditto
MORE_LIBS=${MORE_LIBS-\
    ./libs/android/lib/libsodium.a \
    ./libs/android/lib/libopus.a \
    ./libs/android/lib/libvpx.a \
    ./libs/android/lib/libopenal.a \
    ./libs/android/lib/libfreetype.a}
# WHEEE syslibs
PLATFORM_LIBS=${PLATFORM_LIBS--llog -landroid -lEGL -lGLESv2 -lOpenSLES -lm -lz -ldl}

#
## This is the actual build script. This does all the native compiling and linking.
#
${TOOLCHAIN}/bin/arm-linux-androideabi-gcc \
    -Wno-implicit-function-declaration \
    -Wl,--unresolved-symbols=report-all \
    -I ./toolchain/include \
    -I ./libs/android/include/freetype2/ \
    -I ./libs/android/include/freetype2/freetype/ \
    -I ./libs/android/include/ \
    ${CPPFLAGS} \
    ./src/*.c \
    ./src/ui/*.c \
    ./src/av/*.c \
    ./src/android/*.c \
    ${LDFLAGS} \
    ${TOX_LIBS} \
    ${MORE_LIBS} \
    $ANDROID_NDK_HOME/sources/android/cpufeatures/cpu-features.c \
    -o ./build_android/lib/armeabi/libuTox.so \
    --sysroot=$SYSROOT \
    ${PLATFORM_LIBS} \
    -shared -std=gnu99 -s


$AAPT package -f \
    -M ./src/android/AndroidManifest.xml \
    -S ./src/android/res \
    -I $ANDROID_SDK_HOME/platforms/${SDK_VERSION}/android.jar \
    -F ./build_android/uTox.apk \
    -J ./build_android/java

## Java-8 Version
javac -source 1.7 -target 1.7 -d ./build_android/java ./build_android/java/R.java
## Java-8 Version
# javac -d ./build_android/java ./build_android/java/R.java


$ANDROID_SDK_HOME/build-tools/${DEV_VERSION}/dx \
    --dex \
    --output=./build_android/classes.dex \
    ./build_android/java


java \
    -classpath $ANDROID_SDK_HOME/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain \
    ./build_android/uTox.unsigned.apk \
    -u -z ./build_android/uTox.apk \
    -f ./build_android/classes.dex \
    -nf ./build_android/lib

# Depending on keysize this will be SHA1withDSA or SHA1withRSA
jarsigner \
    -sigalg SHA1withDSA \
    -digestalg SHA1 \
    -keystore ${KEYSTORE} \
    ./build_android/uTox.unsigned.apk \
    utox-dev

mv ./build_android/uTox.unsigned.apk ./build_android/uTox.signed.apk

$ANDROID_SDK_HOME/build-tools/${DEV_VERSION}/zipalign \
    -f 4 \
    ./build_android/uTox.signed.apk \
    ./uTox.ready.apk
