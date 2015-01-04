# Install

- [Unix like](#unix)
- [OS X](#osx)
- [Windows](#windows)
- [Android](#android)
- [Binaries](#binaries)

<a name="unix" />
## Linux / Unix-like

Dependencies:

- Debian Jessie:
  ```bash
  sudo apt-get install libv4l-dev libopenal-dev libfreetype6-dev libdbus-1-dev libxrender-dev libfontconfig1-dev libxext-dev
  ```

- Archlinux:
  ```bash
  sudo pacman -S dbus-c++ fontconfig freetype2 libdbus libvpx libxext libxrender openal v4l-utils
  ```

Compile:
```bash
make all
```

Compile and install:
```bash
sudo make install
```

### Cross-compiling for Windows

Something like this (these commands may sometimes be outdated, try the makefile or make an issue if they do not work):

Windows, using prebuilt toxav dll from Jenkins [32-bit](https://jenkins.libtoxcore.so/job/toxcore_win32_dll/) | [64-bit](https://jenkins.libtoxcore.so/job/toxcore_win64_dll/):

Note: building for Windows requires mingw-w64 (mingw lacks some header files), other compilers (not tested) may work with some tweaks

> windres icons/icon.rc -O coff -o icon.res

> gcc -o uTox.exe *.c ./png/png.c icon.res -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -lopenal32 -lole32 -lstrmiids -loleaut32 -lvpx -ltoxav

### Xlib

> cc -o uTox.o *.c ./png/png.c -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -pthread -lresolv -ldl -lm -lfontconfig -lv4lconvert -lvpx -I/usr/include/freetype2 -ldbus-1

or if you built toxcore statically:

> cc -o uTox.o *.c ./png/png.c -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -lsodium -lopus -lvpx -lm -pthread -lresolv -ldl -lfontconfig -lfreetype -lv4lconvert -I/usr/include/freetype2 -ldbus-1

<a name="osx" />
## OS X
You need XQuartz on 10.8+, no video yet.

> cc -o uTox.o *.c png/png.c -I/opt/X11/include -L/opt/X11/lib -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -framework OpenAL -pthread -lresolv -ldl -lm -lfontconfig -lfreetype -lvpx -I/opt/X11/include/freetype2

If you are relying on Homebrew to provide libraries and headers, you can use the following line instead to save you the trouble:

> cc -o uTox.o *.c png/png.c -L/usr/local/lib -I/usr/local/include -I/opt/X11/include -L/opt/X11/lib -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -framework OpenAL -pthread -lresolv -ldl -lm -lfontconfig -lfreetype -lvpx -I/opt/X11/include/freetype2

<a name="windows" />
## Windows

<a name="android" />
## Android

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

<a name="binaries" />
## Pre-compiled binaries

[Jenkins](https://jenkins.libtoxcore.so) offers automatically compiled binaries. All files below link to the last successful build. [See which changes are in which Jenkins build of uTox](https://jenkins.libtoxcore.so/job/Sync%20uTox/changes).

**Caution**: These are automatically compiled with every push and possibly unstable.

- Linux
  - [amd64](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_amd64/) [[.tar.xz]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_amd64/lastSuccessfulBuild/artifact/utox/utox_linux_amd64.tar.xz)
  - [i686](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_i686/) [[.tar.xz]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_i686/lastSuccessfulBuild/artifact/utox/utox_linux_i686.tar.xz)
- Windows
  - [Updater](https://jenkins.libtoxcore.so/view/Clients/job/utox_update_win32/) (32-bit) [[.zip]](https://jenkins.libtoxcore.so/view/Clients/job/utox_update_win32/lastSuccessfulBuild/artifact/utox-updater.zip)
  - [32-bit](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win32/) [[.zip]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win32/lastSuccessfulBuild/artifact/utox/utox_win32.zip)
  - [64-bit](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win64/) [[.zip]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win64/lastSuccessfulBuild/artifact/utox/utox_win64.zip)
- OS X (not available)
- [Android](https://jenkins.libtoxcore.so/view/Clients/job/uTox_android/) [[.apk]](https://jenkins.libtoxcore.so/job/uTox_android/lastSuccessfulBuild/artifact/utox.apk)
