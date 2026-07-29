# Build

Following are barebone compilation instructions. They probably won't work but #utox on libera.chat can
probably help you out.

If you're looking for it to "just work" you're going to want [these instructions](INSTALL.md).

## Instructions

- [Unix-like](#unix-like)
  * [Linux](#linux)
  * [Ubuntu](#ubuntu)
  * [OpenBSD](#openbsd)
  * [FreeBSD and DragonFlyBSD](#freebsd-and-dragonflybsd)
  * [NetBSD](#netbsd)
- [Windows](#windows)
- [macOS](#macos)
- [Android](#android)

## Unix-like

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

Tested on Windows 10/11 with Cygwin. Dependencies are built from source (no prebuilt zips).

### Prerequisites

1. Install [Cygwin](https://cygwin.com/setup-x86_64.exe) (64-bit).
2. In the installer, select at least these packages:

| Category | Packages |
| --- | --- |
| Devel | `mingw64-x86_64-gcc-core`, `mingw64-x86_64-gcc-g++`, `mingw64-x86_64-headers`, `cmake`, `make`, `autoconf`, `automake`, `libtool`, `pkg-config`, `git`, `yasm`, `nasm` |
| Net | `curl` |
| Interpreters | `perl` |

### Build (x64)

All commands below are run in the **Cygwin Terminal**, from the uTox repository root:

```bash
git clone --recursive https://github.com/uTox/uTox.git
cd uTox
./extra/travis/windows.sh
```

That single script:

1. Builds static dependencies into `$HOME/cache` (libsodium, opus, libvpx, toxcore, OpenAL Soft, filter_audio)
2. Builds `build_win/utox.exe` with MinGW (`cmake/toolchain-win64.cmake`)

Re-running `./extra/travis/windows.sh` reuses the dependency cache when versions are unchanged. To rebuild only µTox after deps are cached:

```bash
./extra/travis/windows-script.sh
```

Portable smoke test:

```bash
./build_win/utox.exe -p
```

### Notes

- Use LF line endings for Autotools sources; the build scripts set `core.autocrlf=false` / `core.eol=lf` for Cygwin/MinGW hosts when cloning deps.
- The binary is linked statically against MinGW pthread so it should not need `libwinpthread-1.dll` at runtime.
- Linux→Windows cross-compiles can use the same scripts (`./extra/travis/windows.sh`) with a MinGW-w64 toolchain installed on the host.

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

Come to think of it, this section is woefully out of date. The android build script in tools/ is likely to be more helpful at this point. Or come to [#utox on libera.chat](https://web.libera.chat/?channels=#utox) and ask for grayhatter. If you're interested in working on android. He'll get you a build environment set up!
