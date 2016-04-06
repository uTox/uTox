#!/usr/bin/env bash
mkdir -p ./build/{lib/armeabi,java}

set -e
set -x

# read settings from a custom settings file.
[ -f settings.android ] && source settings.android

# what is ./lib/arm/lib?
LDFLAGS=${LDFLAGS-L./lib/arm/lib/}

ANDROID_NDK_HOME=${ANDROID_NDK_HOME-/opt/android-ndk}
ANDROID_SDK_HOME=${ANDROID_SDK_HOME-/opt/android-sdk}

SYSROOT=${SYSROOT-/opt/android-ndk/platforms/android-12/arch-arm/}
CPPFLAGS=${CPPFLAGS--I../freetype-arm/include/freetype2/ -I../toxcore-arm/include/}

./toolchain/bin/arm-linux-androideabi-gcc                                           \
    -Wl,--unresolved-symbols=report-all                                             \
    -I ./toolchain/include                                                          \
    -I ./toolchain/sysroot/usr/include                                              \
    -I ./lib/arm/include/freetype2/                                                 \
    -I ./lib/arm/include/freetype2/freetype/                                        \
    -I ./lib/arm/include/                                                           \
    ${CPPFLAGS} \
    ${LDFLAGS} \
    -Ltoolchain/sysroot/usr/lib/ \
    ./src/*.c                                                                       \
    -ltoxcore                                                      \
    -ltoxdns                                                       \
    -ltoxav                                                        \
    -ltoxencryptsave                                               \
    -lsodium                                                       \
    -lopus                                                         \
    -lvpx                                                          \
    -lopenal                                                       \
    -lfreetype                                                     \
    $ANDROID_NDK_HOME/sources/android/cpufeatures/cpu-features.c                     \
    -o ./build/lib/armeabi/libuTox.so                                               \
    --sysroot=$SYSROOT                       \
    -llog -landroid -lEGL -lGLESv2 -lOpenSLES -lm -lz -ldl -shared -std=gnu99 -s


AAPT=${AAPT-/opt/android-sdk/build-tools/23.0.2/aapt}
$AAPT package -f         \
    -M ./src/android/AndroidManifest.xml                    \
    -S ./src/android/res                                    \
    -I $ANDROID_SDK_HOME/platforms/android-23/android.jar    \
    -F ./build/uTox.apk                                     \
    -J ./build/java

javac -d ./build/java ./build/java/R.java

$ANDROID_SDK_HOME/build-tools/23.0.2/dx \
    --dex                              \
    --output=./build/classes.dex       \
    ./build/java


java                                                                                            \
    -classpath $ANDROID_SDK_HOME/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain    \
    ./build/uTox.unsigned.apk                                                                   \
    -u -z ./build/uTox.apk                                                                      \
    -f ./build/classes.dex                                                                      \
    -nf ./build/lib


jarsigner                                   \
    -sigalg SHA1withRSA                     \
    -digestalg SHA1                         \
    -keystore ~/.android/utox.keystore      \
    ./build/uTox.unsigned.apk               \
    utox-dev

mv ./build/uTox.unsigned.apk ./build/uTox.signed.apk

$ANDROID_SDK_HOME/build-tools/23.0.2/zipalign  \
    -f 4                                      \
    ./build/uTox.signed.apk                   \
    ./uTox.ready.apk
