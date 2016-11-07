# Build

Following are barebone compilation instructions that should get you going. They probably wont work but #tox-dev can 
probably help you out if you're nice when you ask.

If you're looking for it to "just work" you're going to want [these instructions](INSTALL.md) instead.

<a name="unix" />
## Unix Like

### Xlib

The easy way out is:
```sh
cd uTox/
make
make install
```

But if the hard way is more your thing, this might work:
```clang -o utox *.c png/png.c -g -Wall -Wshadow -pthread -std=gnu99 `pkg-config --libs --cflags fontconfig freetype2 libtoxav libtoxcore openal vpx x11 xext xrender dbus-1 libv4lconvert filteraudio` -pthread -lm  -lresolv -ldl```

or if you built toxcore statically, less likely to work:

`cc -o uTox.o *.c ./png/png.c -lX11 -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -lsodium -lopus -lvpx -lm -pthread -lresolv -ldl -lfilteraudio -lfontconfig -lfreetype -lv4lconvert -I/usr/include/freetype2 -ldbus-1`

For the build to pass you need to install the following from sources: [filteraudio](https://github.com/irungentoo/filter_audio) [libtoxcore](https://github.com/irungentoo/toxcore)

For base emoji ids support you need: [base_emoji](https://github.com/irungentoo/base_emoji)

<a name="ubuntu15_10">
## Ubuntu
###Tested on Ubuntu 15.10
```bash
sudo apt-get install build-essential libtool autotools-dev automake checkinstall check git yasm libopus-dev libvpx-dev pkg-config libfontconfig1-dev libdbus-1-dev libv4l-dev libxrender-dev libopenal-dev libxext-dev

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


git clone git://github.com/irungentoo/toxcore.git
cd toxcore
autoreconf -i
./configure
make
sudo checkinstall
cd ..

git clone git://github.com/GrayHatter/uTox.git
cd uTox/
make
sudo checkinstall

sudo ldconfig
```

Have fun!

If you're looking for a good IDE, Netbeans is very easy to setup for uTox, in fact, you can just create a new project from the existing sources and everything should work fine.
<a name="win" />
## Windows

### Compiling for Windows

If you have mingw-w64 and a working cygwin enviroment, the build script provided in 
tools/ should just work. You must tell it if you want 32 or 64 bit, then the enviroment
 you want to use.

64bit should be `tools/cygwin-compile.sh`

32bit should be `tools/cygwin-compile.sh -32`

Make sure you grab a copy of toxcore, openal, and filter_audio from 
https://build.tox.chat/ (Make sure you grab the right bit version.)

You can also cross compile from unix if that's more your thing; again you'll need mingw-w64 and then just:

`tools/cygwin-compile.sh -unix`

Don't forget to add -32 if you'd rather build the 32bit version.

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
