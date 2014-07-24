# uTox

Lightweight [Tox](https://github.com/irungentoo/ProjectTox-Core) client.

* Some things are incomplete and there may be some bugs: feel free to make issues/suggestions

* Xlib support is mostly complete, but may have some small bugs (no right click menus, lags)

* Android support is experimental, it lacks important features (accepting/creating file transfers, sending inline images, video)

## Screenshots

uTox running on Windows 8

![test](https://raw.github.com/notsecure/uTox/master/images/uTox-win32.png "uTox running on Windows 8")

uTox running on lubuntu:

![test](https://raw.github.com/notsecure/uTox/master/images/uTox-xlib.png "uTox running on lubuntu")


## Building

Something like this (these commands may sometimes be outdated, try the makefile or make an issue if they do not work):

Windows, using prebuilt toxav dll from Jenkins [32-bit](https://jenkins.libtoxcore.so/job/toxcore_win32_dll/) | [64-bit](https://jenkins.libtoxcore.so/job/toxcore_win64_dll/):

Note: building for Windows requires mingw-w64 (mingw lacks some header files), other compilers (not tested) may work with some tweaks

> windres icons/icon.rc -O coff -o icon.res

> gcc -o uTox.exe *.c ./png/png.c icon.res -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -lopenal32 -lole32 -lstrmiids -loleaut32 -lvpx -ltoxav

Xlib:

> cc -o uTox.o *.c ./png/png.c -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -pthread -lresolv -ldl -lm -lfontconfig -lv4lconvert -lvpx -I/usr/include/freetype2 -ldbus-1

or if you built toxcore statically:

> cc -o uTox.o *.c ./png/png.c -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -lsodium -lopus -lvpx -lm -pthread -lresolv -ldl -lfontconfig -lfreetype -lv4lconvert -I/usr/include/freetype2 -ldbus-1

OS X (you need XQuartz on 10.8+, no video yet):

> cc -o uTox.o *.c -I/opt/X11/include -L/opt/X11/lib -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -framework OpenAL -pthread -lresolv -ldl -lm -lfontconfig -lfreetype -lvpx -I/opt/X11/include/freetype2

Android:

Requires Android SDK+NDK

From uTox root folder, using prebuilt static toxcore + freetype libraries (includes in ../include and libs in ../lib), resulting apk is ./tmp/tmp2.apk:

Setup:

```bash
mkdir ./tmp
mkdir ./tmp/java
mkdir ./tmp/libs
mkdir ./tmp/libs/armeabi
keytool -genkey -v -keystore ./tmp/debug.keystore -alias $ALIAS -keyalg RSA -keysize 2048 -validity 20000
```

Compile + Pack APK

```bash
arm-linux-androideabi-gcc --sysroot=$NDK_PATH/platforms/android-9/arch-arm/ -I../include/freetype2/ -I../include/ ./*.c ./png/png.c -llog -landroid -lEGL -lGLESv2 -lOpenSLES ../lib/libtoxcore.a ../lib/libtoxdns.a ../lib/libtoxav.a ../lib/libsodium.a ../lib/libopus.a ../lib/libvpx.a ../lib/libfreetype.a -lm -lz -ldl -shared -o ./tmp/libs/armeabi/libn.so
/aapt package -f -M ./android/AndroidManifest.xml -S ./android/res -I $SDK_PATH/platforms/android-10/android.jar -F ./tmp/tmp1.apk -J ./tmp/java
javac -d ./tmp/java ./tmp/java/R.java
dx --dex --output=./tmp/classes.dex ./tmp/java
java -classpath $SDK_PATH/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain ./tmp/tmp2.apk -u -z ./tmp/tmp1.apk -f ./tmp/classes.dex -nf ./tmp/libs
jarsigner -sigalg SHA1withRSA -digestalg SHA1 -keystore ./tmp/debug.keystore -storepass $PASSWORD ./tmp/tmp2.apk $ALIAS
```

## Downloads

[Up to date download links on utox.org](http://utox.org)

## Info



