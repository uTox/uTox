# Install

The following steps install μTox on your computron/toaster/carrier pigeon. This guide gets you running ASAP. It mostly deals with precomplied binaries. If you're looking to contribute (you're the real MVP) you're probably looking for [Building](BUILD.md).

For any and all of the following, you'll need to have [toxcore](https://github.com/TokTok/c-toxcore) installed first.

- [Unix-like](#unix-like)
- [macOS](#macOS)
- [Windows](#windows)
- [Android](#android)

## Unix-like

Please make sure you have all of the required [dependencies](DEPENDENCIES.md).

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

### Other distributions

uTox is available in the [Guix](https://guix.gnu.org/) package manager. Install by running:

```bash
guix install utox
```

Refer to the [Guix Manual](https://guix.gnu.org/manual/en/) on how to install and use Guix itself.

## OpenBSD

Right now no one is providing binaries. You will have to compile uTox. See [instructions](BUILD.md#openbsd).

## FreeBSD

You can install uTox using `pkg`.

Install using:

```bash
sudo pkg install utox
```

## macOS

You can download the latest dmg from here: https://github.com/uTox/uTox/releases.

If there is no current version you can try and [build it](BUILD.md#macOS) or ask @publicarray to provide a new build.

Install using homebrew cask:

```bash
brew cask install utox
```

## Windows

Installing on windows isn't really a thing yet... you can download the nighties. They should just work.

  - [32-bit](https://build.tox.chat/job/uTox_build_windows_x86_release/lastSuccessfulBuild/artifact/utox_windows_x86.zip)
  - [64-bit](https://build.tox.chat/job/uTox_build_windows_x86-64_release/lastSuccessfulBuild/artifact/utox_windows_x86-64.zip)
  - Updater (delayed, ask grayhatter for it, and it'll happen)

## Android

Install uTox from the Google Play Store or download the APK: [uTox.apk](https://build.tox.chat/job/uTox_build_android_armhf_release/lastSuccessfulBuild/artifact/uTox.apk)

# Meta
[Jenkins](https://build.tox.chat) offers automatically compiled binaries. All files below link to the last successful build.
