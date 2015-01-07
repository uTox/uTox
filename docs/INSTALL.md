# Install

The following will help you get μTox installed on your computron/toaster/carrier pigon. The focus of this guide is to 
get you running ASAP. And mostly deals with precomplied binarys. If you're looking to contribute (you're the real MVP) 
you're probably looking for [Building](docs/BUILD.md). 

- [Unix-like](#unix)
- [OS X](#osx)
- [Windows](#windows)
- [Android](#android)

<a name="unix" />
## Unix-like

Dependencies:

```dbus freetype libvpx openal v4l xext xrender```

Compile:
```bash
make all
```

Now install:
```bash
sudo make install
```

If make/install really isn't your thing, you can try some precomplied binarys.
- [amd64](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_amd64/) [[.tar.xz]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_amd64/lastSuccessfulBuild/artifact/utox/utox_linux_amd64.tar.xz)
- [i686](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_i686/) [[.tar.xz]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_linux_i686/lastSuccessfulBuild/artifact/utox/utox_linux_i686.tar.xz)


- Debian Jessie:
  ```bash
  sudo apt-get install libdbus-1-dev libfontconfig1-dev libfreetype6-dev libopenal-dev libv4l-dev libxext-dev libxrender-dev
  ```

- Archlinux:
  ```bash
  sudo pacman -S dbus-c++ fontconfig freetype2 libdbus libvpx libxext libxrender openal v4l-utils
  ```
    1. Note: `dbus-c++` is an optional dependency.
    2. Please note that [`tox-git`](https://aur.archlinux.org/packages/tox-git/) package from AUR is also required (unless you have already built `toxcore` from source).


<a name="osx" />
## OS X

No one is currently providing binaries for OSX yet... Sorry Apple people...

[I guess I'll try to build it](docs/BUILD.md#OSX).

<a name="windows" />
## Windows

Installing on windows isn't really a thing yet... you can download the nightlies. They should just work.

  - [32-bit](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win32/) [[.zip]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win32/lastSuccessfulBuild/artifact/utox/utox_win32.zip)
  - [64-bit](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win64/) [[.zip]](https://jenkins.libtoxcore.so/view/Clients/job/uTox_win64/lastSuccessfulBuild/artifact/utox/utox_win64.zip)
  - [Updater](https://jenkins.libtoxcore.so/view/Clients/job/utox_update_win32/) (32-bit) [[.zip]](https://jenkins.libtoxcore.so/view/Clients/job/utox_update_win32/lastSuccessfulBuild/artifact/utox-updater.zip)

<a name="android" />
## Android

μTox on android has been neglected too long, so we're not currently providing binaries.

# Meta
[Jenkins](https://jenkins.libtoxcore.so) offers automatically compiled binaries. All files below link to the last successful build. [See which changes are in which Jenkins build of uTox](https://jenkins.libtoxcore.so/job/Sync%20uTox/changes).