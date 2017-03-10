# Install

The following steps install μTox on your computron/toaster/carrier pigeon. This guide gets you running ASAP. It mostly deals with precomplied binaries. If you're looking to contribute (you're the real MVP) you're probably looking for [Building](BUILD.md).

For any and all of the following, you'll need to have [toxcore](https://github.com/TokTok/c-toxcore) installed first.

- [Unix-like](#unix)
- [OS X](#osx)
- [Windows](#windows)
- [Android](#android)

<a name="unix" />
## Unix-like

Dependencies:

```dbus filter_audio freetype libvpx openal v4l xext xrender cmake```

1. First compile:

    ```bash
    mkdir build
    cd build
    cmake ..
    make all
    ```

2. Then install:

    ```bash
    sudo make install
    ```

If make/install really isn't your thing, you can try some precomplied binaries.
- [amd64](https://build.tox.chat/job/uTox_build_linux_x86-64_release/lastSuccessfulBuild/artifact/utox_linux_x86-64.tar.xz)
- [i686](https://build.tox.chat/job/uTox_build_linux_x86_release/lastSuccessfulBuild/artifact/utox_linux_x86.tar.xz)

### Adding a desktop launcher

Assuming that repository working copy is your current directory.

1. Copy `.desktop` entry

    ```bash
    sudo cp ./utox.desktop /usr/share/applications/utox.desktop
    ```

2. Make it executable

    ```bash
    sudo chmod +x /usr/share/applications/utox.desktop
    ```

3. Add an icon

    ```bash
    sudo cp ./icons/utox-128x128.png /usr/share/pixmaps/utox.png
    ```

### Archlinux

If you're lucky enough to use Archlinux, someone has an AUR package just for you!

https://aur.archlinux.org/packages/utox-git/

<a name="osx" />
## OS X

No one is currently providing binaries for OSX yet... Sorry Apple people... you should ask @stal888 to do something about that!

[I guess I'll try to build it](BUILD.md#OSX).

<a name="windows" />
## Windows

Installing on windows isn't really a thing yet... you can download the nighties. They should just work.

  - [32-bit](https://build.tox.chat/job/uTox_build_windows_x86_release/lastSuccessfulBuild/artifact/utox_windows_x86.zip)
  - [64-bit](https://build.tox.chat/job/uTox_build_windows_x86-64_release/lastSuccessfulBuild/artifact/utox_windows_x86-64.zip)
  - Updater (delayed, ask grayhatter for it, and it'll happen)

<a name="android" />
## Android

[uTox.apk](https://build.tox.chat/job/uTox_build_android_armhf_release/lastSuccessfulBuild/artifact/uTox.apk)

# Meta
[Jenkins](https://build.tox.chat) offers automatically compiled binaries. All files below link to the last successful build.
