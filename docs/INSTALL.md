# Install

The following steps install μTox on your computron/toaster/carrier pigeon. This guide gets you running ASAP. It mostly deals with precomplied binaries. If you're looking to contribute (you're the real MVP) you're probably looking for [Building](BUILD.md).

For any and all of the following, you'll need to have [toxcore](https://github.com/TokTok/c-toxcore) installed first.

- [Windows](#windows)
- [Unix-like](#unix-like)
  - [Archlinux](#archlinux)
  - [Slackware](#slackware)
  - [Other distributions](#other-distributions)
- [OpenBSD](#openBSD)
- [FreeBSD](#freeBSD)
- [macOS](#macOS)
- [Android](#android)

## Windows

Installing on Windows isn't really a thing yet... just download and run the latest version: [x64](https://github.com/uTox/uTox/releases/download/v0.17.2/utox_x86_64.exe).

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

If make/install really isn't your thing, you can try some precomplied binaries:

- [x86-64](https://build.tox.chat/job/uTox_build_linux_x86-64_release/lastSuccessfulBuild/artifact/utox_linux_x86-64.tar.xz)
- [i686](https://build.tox.chat/job/uTox_build_linux_x86_release/lastSuccessfulBuild/artifact/utox_linux_x86.tar.xz)

### Archlinux

If you're lucky enough to use Archlinux, uTox is in the community repo:

- [x86-64](https://www.archlinux.org/packages/community/x86_64/utox/)
- [i686](https://www.archlinux.org/packages/community/i686/utox/)

Install by running:

```bash
sudo pacman -S utox
```

### Slackware

If you use Slackware you can download the slack build from [here](https://slackbuilds.org/repository/14.2/network/uTox/).

### Other distributions

uTox is available in the [Guix](https://guix.gnu.org/) package manager. Install by running:

```bash
guix install utox
```

Refer to the [Guix Manual](https://guix.gnu.org/manual/en/) on how to install and use Guix itself.

## OpenBSD

Right now no one is providing binaries. You will have to compile uTox. See [instructions](BUILD.md#openbsd).

## FreeBSD

You can install uTox using `pkg`:

```bash
sudo pkg install utox
```

## macOS

If there is no current version [here](https://github.com/uTox/uTox/releases/latest) you can try to [build it](BUILD.md#macOS).

Install using homebrew cask:

```bash
brew cask install utox
```


## Android

Install uTox from [Google Play](https://play.google.com/apps/testing/tox.client.utox) or download the [APK](https://build.tox.chat/job/uTox_build_android_armhf_release/lastSuccessfulBuild/artifact/uTox.apk).
