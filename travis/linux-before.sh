#!/bin/sh

set -e -x

get() {
    mkdir -p TRAVIS_PREFIX
    curl "$1" | tar -C TRAVIS_PREFIX -xJ
}

get https://build.tox.chat/view/libsodium/job/libsodium_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libsodium.tar.xz
get https://build.tox.chat/job/libvpx_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libvpx.tar.xz
get https://build.tox.chat/job/libtoxcore_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libtoxcore.tar.xz
get https://build.tox.chat/job/libfilteraudio_build_linux_x86-64_mixed_release/lastSuccessfulBuild/artifact/libfilteraudio.tar.xz
get https://build.tox.chat/job/libopus_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libopus.tar.xz

NEW_PREFIX="$(pwd)/TRAVIS_PREFIX"
find TRAVIS_PREFIX -name "*.pc" | while read FILENAME; do
    sed -E -e 's|^prefix=.*|prefix='$NEW_PREFIX'|' -i $FILENAME
done


PKG_CONFIG_PATH="./TRAVIS_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH" pkg-config --list-all
PKG_CONFIG_PATH="./TRAVIS_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH" echo $PKG_CONFIG_PATH
PKG_CONFIG_PATH="./TRAVIS_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH" ls -la ./*/*/*
