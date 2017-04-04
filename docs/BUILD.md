# Build

Following are barebone compilation instructions. They probably wont work but #utox on freenode can
probably help you out.

If you're looking for it to "just work" you're going to want [these instructions](INSTALL.md).

<a name="unix" />
## Unix Like

### Linux

The easy way out is:
```sh
git clone git://github.com/uTox/uTox.git
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

or if you built toxcore statically:
```sh
git clone git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake -DTOXCORE_STATIC=ON ..
make
make install
```

For the build to pass you need to install the following from sources: [filteraudio](https://github.com/irungentoo/filter_audio) [libtoxcore](https://github.com/TokTok/c-toxcore)

For base emoji ids support you need: [base_emoji](https://github.com/irungentoo/base_emoji)

<a name="ubuntu15_10">
## Ubuntu
### Tested on Ubuntu 15.10
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

git clone git://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake ..
make
sudo checkinstall
```

Have fun!

If you're looking for a good IDE, Netbeans is very easy to set up for uTox. In fact, you can just create a new project from the existing sources and everything should work fine.
<a name="win" />
## Windows

### Compiling for Windows

Dependencies:

|   Name       | Required |
|--------------|----------|
| cmake >= 3.2 |   yes    |
| filter_audio |   no     |
| libvpx       |   yes    |
| openal       |   yes    |
| opus         |   yes    |
| toxcore      |   yes    |

The dependencies can be downloaded from here: https://build.tox.chat/ (Make sure you grab the right bit version.) All the libraries should be place in $UTOX_ROOT/libs/windows-x64/.

You will need a working Cygwin environment or Unix desktop to compile windows.

For 32 bit:
```bash
git clone https://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="../cmake/toolchain-win32.cmake" -DTOXCORE_STATIC=ON ..
make
```

For 64 bit:
```bash
git clone https://github.com/uTox/uTox.git
cd uTox/
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="../cmake/toolchain-win64.cmake" -DTOXCORE_STATIC=ON ..
make
```

<a name="osx" />
## OSX

See [COCOA.md](COCOA.md).

<a name="and" />
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
arm-linux-androideabi-gcc --sysroot=$NDK_PATH/platforms/android-9/arch-arm/ -I../include/freetype2/ -I../include/ ./*.c ./png/png.c -llog -landroid -lEGL -lGLESv2 -lOpenSLES ../lib/libtoxcore.a ../lib/libtoxdns.a ../lib/libtoxav.a ../lib/libsodium.a ../lib/libopus.a ../lib/libvpx.a ../lib/libfreetype.a -lm -lz -ldl -shared -o ./tmp/libs/armeabi/libn.so
/aapt package -f -M ./android/AndroidManifest.xml -S ./android/res -I $SDK_PATH/platforms/android-10/android.jar -F ./tmp/tmp1.apk -J ./tmp/java
javac -d ./tmp/java ./tmp/java/R.java
dx --dex --output=./tmp/classes.dex ./tmp/java
java -classpath $SDK_PATH/tools/lib/sdklib.jar com.android.sdklib.build.ApkBuilderMain ./tmp/tmp2.apk -u -z ./tmp/tmp1.apk -f ./tmp/classes.dex -nf ./tmp/libs
jarsigner -sigalg SHA1withRSA -digestalg SHA1 -keystore ./tmp/debug.keystore -storepass $PASSWORD ./tmp/tmp2.apk $ALIAS
```

Come to think of it, this section is woahfully out of date. The android build script in tools/ is likely to be more helpful at this point. Or come to #tox-dev and ask for grayhatter. If you're interested in working on android. He'll get you a build enviroment set up!
