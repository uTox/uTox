# Build

Following are barebone compilation instructions. They probably won't work but #utox on freenode can
probably help you out.

If you're looking for it to "just work" you're going to want [these instructions](INSTALL.md).

## Instructions

- [Unix Like](#unix-like)
  * [Linux](#linux)
  * [Ubuntu](#ubuntu)
  * [OpenBSD](#openbsd)
  * [FreeBSD and DragonFlyBSD](#freebsd-and-dragonflybsd)
  * [NetBSD](#netbsd)
- [Windows](#windows)
- [macOS](#macos)
- [Android](#android)

## Unix Like

### Linux

Before compiling make sure you have all of the [dependencies](DEPENDENCIES.md#linux) installed.

The easy way out is:
```sh
git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake ..
make
make install
```

> To build the binary with debug symbols (e.g. for debugging with gdb) you should append the `-DCMAKE_BUILD_TYPE=Debug` option to the `cmake ..` command above.
>
> In that case you want to set the env variable  `ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer`  for the address sanitizer (ASAN) to show nicer stack traces.
> See <http://clang.llvm.org/docs/AddressSanitizer.html#symbolizing-the-reports> for more details.

or if you want to link toxcore statically:
```sh
git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake -DSTATIC_TOXCORE=ON ..
make
make install
```

For the build to pass you need to install the following from sources: [filteraudio](https://github.com/irungentoo/filter_audio) [libtoxcore](https://github.com/TokTok/c-toxcore)

For base emoji ids support you need: [base_emoji](https://github.com/irungentoo/base_emoji)

#### musl + clang

If you use clang on a musl system, you may need to disable link-time optimizations, in case you get linking errors like the following:
```
/bin/x86_64-unknown-linux-musl-ld: src/av/libutoxAV.a: error adding symbols: archive has no index; run ranlib to add one
clang-9: error: linker command failed with exit code 1 (use -v to see invocation)
```
In that case, you need to pass `-DENABLE_LTO=OFF` to cmake.

### Ubuntu

Tested on Ubuntu 18.04

```bash
sudo apt-get install build-essential libtool autotools-dev automake checkinstall check git yasm libopus-dev libvpx-dev pkg-config libfontconfig1-dev libdbus-1-dev libv4l-dev libxrender-dev libopenal-dev libxext-dev cmake

git clone git://github.com/jedisct1/libsodium.git
cd libsodium
git checkout tags/1.0.3
./autogen.sh
./configure && make check
sudo checkinstall
cd ..


git clone git://github.com/irungentoo/filter_audio.git
cd filter_audio
make
sudo checkinstall
cd ..


git clone git://github.com/TokTok/c-toxcore.git
cd c-toxcore
cmake .
make
sudo checkinstall
cd ..

sudo ldconfig

git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake ..
make
sudo checkinstall
```

Have fun!

If you're looking for a good IDE, Netbeans is very easy to set up for uTox. In fact, you can just create a new project from the existing sources and everything should work fine.

### OpenBSD

First install the [dependencies](DEPENDENCIES.md#openbsd-and-netbsd):

```bash
doas pkg_add openal cmake libv4l toxcore git check
```

Optionally install D-Bus and GTK+3:
```bash
doas pkg_add dbus gtk+3
```

Now compile uTox:

```bash
git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake ..
make -j `sysctl -n hw.ncpu`
make test
doas make install
```

### FreeBSD and DragonFlyBSD

Install the [dependencies](DEPENDENCIES.md#freebsd-and-dragonflybsd):

```bash
sudo pkg install libv4l v4l_compat openal-soft toxcore git check
```

Optionally install D-Bus, GTK+3 and filteraudio:
```bash
sudo pkg install dbus libfilteraudio gtk3
```

Now compile uTox:

```bash
git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake ..
make
make test
sudo make install
```

### NetBSD

Install the [dependencies](DEPENDENCIES.md#openbsd-and-netbsd):

```bash
sudo pkgin install openal-soft cmake libv4l toxcore git check
```

Optionally install D-Bus and GTK+3:
```base
sudo pkgin install dbus gtk3
```

Now compile uTox:
```bash
git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake ..
make
make test
sudo make install
```

## Windows

Tested on Windows 10.

You will need a working Cygwin environment.

- Download Cygwin ([x86](https://cygwin.com/setup-x86.exe)/[x64](https://cygwin.com/setup-x86_64.exe))
- Search and select exactly these packages in Devel category:
  - mingw64-i686-gcc-core (x86) / mingw64-x86_64-gcc-core (x64)
  - make
  - cmake

All following commands should be executed in Cygwin Terminal.

```bash
cd /cygdrive/c
mkdir projects
cd projects/
git clone --recursive git://github.com/uTox/uTox.git
cd uTox/
mkdir libs
cd libs/
```

`mkdir windows-x32` or `mkdir windows-x64`

```
cd ../uTox/
mkdir build
cd build
```

Download .zip files and place them into `windows-x32` or `windows-x64` folder.
Extract here with your archiver and merge when it'll ask for replacement:

- toxcore ([x86](https://build.tox.chat/view/libtoxcore/job/libtoxcore-toktok_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libtoxcore-toktok_build_windows_x86_static_release.zip)/[x64](https://build.tox.chat/view/libtoxcore/job/libtoxcore-toktok_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libtoxcore-toktok_build_windows_x86-64_static_release.zip))
- openal ([x86](https://build.tox.chat/view/libopenal/job/libopenal_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libopenal_build_windows_x86_static_release.zip)/[x64](https://build.tox.chat/view/libopenal/job/libopenal_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libopenal_build_windows_x86-64_static_release.zip))
- sodium ([x86](https://build.tox.chat/view/libsodium/job/libsodium_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libsodium_build_windows_x86_static_release.zip)/[x64](https://build.tox.chat/view/libsodium/job/libsodium_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libsodium_build_windows_x86-64_static_release.zip))
- libvpx ([x86](https://build.tox.chat/view/libvpx/job/libvpx_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libvpx_build_windows_x86_static_release.zip)/[x64](https://build.tox.chat/view/libvpx/job/libvpx_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libvpx_build_windows_x86-64_static_release.zip))
- opus ([x86](https://build.tox.chat/view/libopus/job/libopus_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libopus_build_windows_x86_static_release.zip)/[x64](https://build.tox.chat/view/libopus/job/libopus_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libopus_build_windows_x86-64_static_release.zip))
- filter_audio ([x86](https://build.tox.chat/view/libfilteraudio/job/libfilteraudio_build_windows_x86_static_release/lastSuccessfulBuild/artifact/libfilteraudio.zip)/[x64](https://build.tox.chat/view/libfilteraudio/job/libfilteraudio_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libfilteraudio.zip))

And go back to terminal (make sure you're still in `build` folder):

- For x86:
    ```bash
    cmake -DCMAKE_TOOLCHAIN_FILE="../cmake/toolchain-win32.cmake" -DSTATIC_TOXCORE=ON -DCMAKE_BUILD_TYPE=Release ..
    make
    ```

- For x64:
    ```bash
    cmake -DCMAKE_TOOLCHAIN_FILE="../cmake/toolchain-win64.cmake" -DSTATIC_TOXCORE=ON -DCMAKE_BUILD_TYPE=Release ..
    make
    ```

## macOS

```bash
brew tap tox/tox
brew install --HEAD utox
```

For details see [COCOA.md](COCOA.md).

## Android

Requires Android SDK+NDK

From uTox root folder, using prebuilt static toxcore + freetype libraries (includes in ../include and libs in ../lib), resulting apk is ./tmp/tmp2.apk:

### Setup:

```bash
mkdir ./tmp
mkdir ./tmp/java
mkdir ./tmp/libs
mkdir ./tmp/libs/armeabi
keytool -genkey -v -keystore ./tmp/debug.keystore -alias $ALIAS -keyalg RSA -keysize 2048 -validity 20000
```

### Compile + Pack APK

```bash
arm-linux-androideabi-gcc --sysroot=$NDK_PATH/platforms/android-9/arch-arm/ -I../include/freetype2/ -I../include/ ./*.c ./png/png.c -llog -landroid -lEGL -lGLESv2 -lOpenSLES ../lib/libtoxcore.a ../lib/libtoxav.a ../lib/libsodium.a ../lib/libopus.a ../lib/libvpx.a ../lib/libfreetype.a -lm -lz -ldl -shared -o ./tmp/libs/armeabi/libn.so
/aapt package -f -M ./android/AndroidManifest.xml -S ./android/res -I $SDK_PATH/platforms/android-10/android.jar -F ./tmp/tmp1.apk -J ./tmp/java
javac -d ./tmp/java ./tmp/java/R.java
dx --dex --output=./tmp/classes.dex ./tmp/java
java -classpath $SDK_PATH/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain ./tmp/tmp2.apk -u -z ./tmp/tmp1.apk -f ./tmp/classes.dex -nf ./tmp/libs
jarsigner -sigalg SHA1withRSA -digestalg SHA1 -keystore ./tmp/debug.keystore -storepass $PASSWORD ./tmp/tmp2.apk $ALIAS
```

Come to think of it, this section is woefully out of date. The android build script in tools/ is likely to be more helpful at this point. Or come to [#utox on Freenode](https://webchat.freenode.net/?channels=#utox) and ask for grayhatter. If you're interested in working on android. He'll get you a build environment set up!
