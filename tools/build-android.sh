#!/usr/bin/env bash
mkdir -p ./build/{lib/armeabi,java}

set -e
set -x

# read settings from a custom settings file.
[ -f settings.android ] && source settings.android


# ./lib/arm/lib is the compilation of all the required dependencies you can scrape from build.tox.chat needed to
# cross compile uTox to Android, you might choose to store them elsewhere.
LDFLAGS+=" -L./lib/arm/lib/"

DEV_VERSION="23.0.2"
SDK_VERSION="android-23"
NDK_VERSION="android-12"

KEYSTORE=${KEYSTORE-~/.android/utox.keystore}

# Standard dev kit locations on posix
ANDROID_NDK_HOME=${ANDROID_NDK_HOME-/opt/android-ndk}
ANDROID_SDK_HOME=${ANDROID_SDK_HOME-/opt/android-sdk}
SYSROOT=${SYSROOT-/opt/android-ndk/platforms/${NDK_VERSION}/arch-arm}
AND_LINKS=${AND_LINKS-" ./lib/arm/lib/lib*.a"}
TOOLCHAIN=${TOOLCHAIN-./toolchain}
AAPT=${AAPT-/opt/android-sdk/build-tools/${DEV_VERSION}/aapt}

[ -d ${TOOLCHAIN} ] || "$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh"     \
        --ndk-dir="$ANDROID_NDK_HOME"                                                   \
        --toolchain="arm-linux-androideabi-clang"                                       \
        --install-dir=$TOOLCHAIN/                                                       \
        --platform=${NDK_VERSION}

${TOOLCHAIN}/bin/arm-linux-androideabi-gcc                                              \
    -Wl,--unresolved-symbols=report-all                                             \
    -I ./toolchain/include                                                          \
    -I ./lib/arm/include/freetype2/                                                 \
    -I ./lib/arm/include/freetype2/freetype/                                        \
    -I ./lib/arm/include/                                                           \
    ./src/*.c                                                                       \
    ./lib/arm/lib/libtoxcore.a                                                      \
    ./lib/arm/lib/libtoxdns.a                                                       \
    ./lib/arm/lib/libtoxav.a                                                        \
    ./lib/arm/lib/libtoxencryptsave.a                                               \
    ./lib/arm/lib/libsodium.a                                                       \
    ./lib/arm/lib/libopus.a                                                         \
    ./lib/arm/lib/libvpx.a                                                          \
    ./lib/arm/lib/libopenal.a                                                       \
    ./lib/arm/lib/libfreetype.a                                                     \
    $ANDROID_NDK_HOME/sources/android/cpufeatures/cpu-features.c                    \
    -o ./build/lib/armeabi/libuTox.so                                               \
    --sysroot=$SYSROOT                                                              \
    -llog -landroid -lEGL -lGLESv2 -lOpenSLES -lm -lz -ldl -shared -std=gnu99 -s

$AAPT package -f                                                \
    -M ./src/android/AndroidManifest.xml                        \
    -S ./src/android/res                                        \
    -I $ANDROID_SDK_HOME/platforms/${SDK_VERSION}/android.jar   \
    -F ./build/uTox.apk                                         \
    -J ./build/java

javac -d ./build/java ./build/java/R.java

$ANDROID_SDK_HOME/build-tools/23.0.2/dx \
    --dex                               \
    --output=./build/classes.dex        \
    ./build/java


java                                                                                            \
    -classpath $ANDROID_SDK_HOME/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain   \
    ./build/uTox.unsigned.apk                                                                   \
    -u -z ./build/uTox.apk                                                                      \
    -f ./build/classes.dex                                                                      \
    -nf ./build/lib


jarsigner                       \
    -sigalg SHA1withRSA         \
    -digestalg SHA1             \
    -keystore ${KEYSTORE}       \
    ./build/uTox.unsigned.apk   \
    utox-dev

mv ./build/uTox.unsigned.apk ./build/uTox.signed.apk

$ANDROID_SDK_HOME/build-tools/${DEV_VERSION}/zipalign   \
    -f 4                                                \
    ./build/uTox.signed.apk                             \
    ./uTox.ready.apk
