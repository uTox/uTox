# Install

The following steps install μTox on your computron/toaster/carrier pigeon. This guide gets you running ASAP. It mostly deals with precomplied binaries. If you're looking to contribute (you're the real MVP) you're probably looking for [Building](BUILD.md).

For any and all of the following, you'll need to have [toxcore](https://github.com/TokTok/c-toxcore) installed first.

- [Unix-like](#unix-like)
- [OS X](#os-x)
- [Windows](#windows)
- [Android](#android)

## Unix-like

Dependencies:

|   Name       | Required |
|--------------|----------|
| cmake >= 3.2 |   yes    |
| dbus         |   no     |
| filter_audio |   no     |
| freetype     |   yes    |
| GTK          |   no (runtime only) |
| libvpx       |   yes    |
| openal       |   yes    |
| toxcore      |   yes    |
| v4l          |   yes    |
| xext         |   yes    |
| xrender      |   yes    |

1. First compile:

    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

2. Then install:

    ```bash
    sudo make install
    ```

If make/install really isn't your thing, you can try some precomplied binaries.
- [amd64](https://build.tox.chat/job/uTox_build_linux_x86-64_release/lastSuccessfulBuild/artifact/utox_linux_x86-64.tar.xz)
- [i686](https://build.tox.chat/job/uTox_build_linux_x86_release/lastSuccessfulBuild/artifact/utox_linux_x86.tar.xz)

### Archlinux

If you're lucky enough to use Archlinux, uTox is in the community repo.
 - [x86_64](https://www.archlinux.org/packages/community/x86_64/utox/)
 - [i686](https://www.archlinux.org/packages/community/i686/utox/)

Install by running:

```bash
sudo pacman -S utox
```

### Slackware

If you use Slackware you can download the slack build from here: https://slackbuilds.org/repository/14.2/network/uTox/

## OpenBSD

Right now no one is providing binaries. You will have to compile uTox. See [instructions](BUILD.md#openbsd).

## OS X

No one is currently providing binaries for OSX yet... Sorry Apple people... you should ask @stal888 to do something about that!

[I guess I'll try to build it](BUILD.md#osx).

## Windows

Installing on windows isn't really a thing yet... you can download the nighties. They should just work.

  - [32-bit](https://build.tox.chat/job/uTox_build_windows_x86_release/lastSuccessfulBuild/artifact/utox_windows_x86.zip)
  - [64-bit](https://build.tox.chat/job/uTox_build_windows_x86-64_release/lastSuccessfulBuild/artifact/utox_windows_x86-64.zip)
  - Updater (delayed, ask grayhatter for it, and it'll happen)

## Android

Install uTox from the Google Play Store or download the APK: [uTox.apk](https://build.tox.chat/job/uTox_build_android_armhf_release/lastSuccessfulBuild/artifact/uTox.apk)

# Meta
[Jenkins](https://build.tox.chat) offers automatically compiled binaries. All files below link to the last successful build.
