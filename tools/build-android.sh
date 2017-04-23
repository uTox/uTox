#!/usr/bin/env bash
# read settings from a custom settings file.
[ -f settings.android ] && source settings.android

set -ex



# You may need to change these values, to what ever your system has available
DEV_VERSION="25.0.0"
SDK_VERSION="android-23"
NDK_VERSION="android-12"

# $TOXCLIORE_LIBS is the compilation of all the required dependencies you can
# scrape from build.tox.chat needed to cross compile uTox to Android, you
# might choose to store them elsewhere.
TOXCORE_LIBS=${TOXCORE_LIBS-./libs/android/lib}
LDFLAGS=${LDFLAGS--L$TOXCORE_LIBS/}
TOOLCHAIN=${TOOLCHAIN-./toolchain}

BUILD_DIR=${BUILD_DIR-./build_android}

# Standard dev kit locations on posix
ANDROID_NDK_HOME=${ANDROID_NDK_HOME-/opt/android-ndk}
ANDROID_SDK_HOME=${ANDROID_SDK_HOME-/opt/android-sdk}

KEYSTORE=${KEYSTORE-~/.android/utox.keystore}

SYSROOT=${SYSROOT-${ANDROID_NDK_HOME}/platforms/${NDK_VERSION}/arch-arm}

AAPT=${AAPT-${ANDROID_SDK_HOME}/build-tools/${DEV_VERSION}/aapt}
DX=${DX-${ANDROID_SDK_HOME}/build-tools/${DEV_VERSION}/dx}
ZIPALIGN=${ZIPALIGN-$ANDROID_SDK_HOME/build-tools/${DEV_VERSION}/zipalign}

mkdir -p ${BUILD_DIR}/{lib/armeabi,java}
mkdir -p ./.android/


if [ $1 == "--new" ]; then
    rm ${BUILD_DIR}/lib/armeabi/libuTox.so || true
fi

if [ $1 == "--auto-CI" ]; then
    curl -O https://utox.io/android.tar.gz
    tar xf android.tar.gz
fi

[ -d ${TOOLCHAIN} ] || "$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh" \
        --toolchain="arm-linux-androideabi-clang" \
        --install-dir=${TOOLCHAIN}/ \
        --platform=${NDK_VERSION}

TOX_LIBS=${TOX_LIBS-\
    $TOXCORE_LIBS/libtoxcore.a \
    $TOXCORE_LIBS/libtoxdns.a \
    $TOXCORE_LIBS/libtoxav.a \
    $TOXCORE_LIBS/libtoxencryptsave.a }

MORE_LIBS=${MORE_LIBS-\
    $TOXCORE_LIBS/libsodium.a \
    $TOXCORE_LIBS/libopus.a \
    $TOXCORE_LIBS/libvpx.a \
    $TOXCORE_LIBS/libopenal.a \
    $TOXCORE_LIBS/libfreetype.a }

PLATFORM_LIBS=${PLATFORM_LIBS--llog -landroid -lEGL -lGLESv2 -lOpenSLES -lm -lz -ldl}

if ! [ -f ${BUILD_DIR}/lib/armeabi/libuTox.so ]; then
    ${TOOLCHAIN}/bin/arm-linux-androideabi-clang -std=gnu11 \
        -Wformat=0 \
        -Wl,--unresolved-symbols=report-all \
        -I ./toolchain/include \
        -I ./libs/android/include/freetype2/ \
        -I ./libs/android/include/ \
        -I ./sys/ \
        ${CFLAGS} \
        ./src/*.c \
        ./src/ui/*.c \
        ./src/av/*.c \
        ./src/layout/*.c \
        ./src/android/*.c \
        ./toxcore/toxcore/*.c \
        ./toxcore/toxav/*.c \
        ./toxcore/toxencryptsave/*.c \
        ./toxcore/toxdns/*.c \
        $ANDROID_NDK_HOME/sources/android/cpufeatures/cpu-features.c \
        ${LDFLAGS} \
        ${MORE_LIBS} \
        -o ${BUILD_DIR}/lib/armeabi/libuTox.so \
        --sysroot=$SYSROOT \
        ${PLATFORM_LIBS} \
        -DPLATFORM_ANDROID=1 \
        -shared -s
fi

$AAPT package -f \
    -M ./src/android/AndroidManifest.xml \
    -S ./src/android/res \
    -I $ANDROID_SDK_HOME/platforms/${SDK_VERSION}/android.jar \
    -F ${BUILD_DIR}/uTox.apk \
    -J ${BUILD_DIR}/java

javac \
    -d ${BUILD_DIR}/java \
    -source 7 \
    -target 7 \
    ${BUILD_DIR}/java/R.java

$DX --dex \
    --output="${BUILD_DIR}/classes.dex" \
    ${BUILD_DIR}/java

# the class path is likely hacky, but I can't be arsed to find the real fix now
java \
    -classpath $ANDROID_SDK_HOME/tools/lib/sdklib-25.3.0.jar \
    com.android.sdklib.build.ApkBuilderMain \
    ${BUILD_DIR}/uTox.unsigned.apk \
    -u -z ${BUILD_DIR}/uTox.apk \
    -f ${BUILD_DIR}/classes.dex \
    -nf ${BUILD_DIR}/lib

if [ "$1" == "--auto-CI"  ]; then
    keytool -genkeypair -v \
        -dname "cn=uToxer, ou=uTox, o=Tox, c=US" \
        -keystore ./tmp.keystore \
        -keyalg RSA \
        -keysize 2048 \
        -validity 36500 \
        -alias "utox-default" \
        -keypass   "the default password...really?" \
        -storepass "the default password...really?"

    jarsigner \
        -sigalg SHA1withRSA \
        -digestalg SHA1 \
        -keystore ./tmp.keystore \
        ${BUILD_DIR}/uTox.unsigned.apk \
        -keypass   "the default password...really?" \
        -storepass "the default password...really?" \
        "utox-default"
else
    jarsigner \
        -sigalg SHA1withRSA \
        -digestalg SHA1 \
        -keystore ${KEYSTORE} \
        ${BUILD_DIR}/uTox.unsigned.apk \
        utox-dev
fi

mv ${BUILD_DIR}/uTox.unsigned.apk ${BUILD_DIR}/uTox.signed.apk

$ZIPALIGN \
    -f 4 \
    ${BUILD_DIR}/uTox.signed.apk \
    ./uTox.ready.apk
