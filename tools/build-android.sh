#!/usr/bin/bash
mkdir -p ./build/{lib/armeabi,java}

set -e

./toolchain/bin/arm-linux-androideabi-gcc                                           \
    -Wl,--unresolved-symbols=report-all                                             \
    -I ./toolchain/                                                                 \
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
    /opt/android-ndk/sources/android/cpufeatures/cpu-features.c                     \
    -o ./build/lib/armeabi/libuTox.so                                               \
    --sysroot=/opt/android-ndk/platforms/android-12/arch-arm/                       \
    -llog -landroid -lEGL -lGLESv2 -lOpenSLES -lm -lz -ldl -shared -std=gnu99 -s


/opt/android-sdk/build-tools/23.0.2/aapt package -f         \
    -M ./src/android/AndroidManifest.xml                    \
    -S ./src/android/res                                    \
    -I /opt/android-sdk/platforms/android-23/android.jar    \
    -F ./build/uTox.apk                                     \
    -J ./build/java

javac -d ./build/java ./build/java/R.java

/opt/android-sdk/build-tools/23.0.2/dx \
    --dex                              \
    --output=./build/classes.dex       \
    ./build/java


java                                                                                            \
    -classpath /opt/android-sdk/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain    \
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

/opt/android-sdk/build-tools/23.0.2/zipalign  \
    -f 4                                      \
    ./build/uTox.signed.apk                   \
    ./uTox.ready.apk
