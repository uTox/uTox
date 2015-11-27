#!/bin/sh

get() {
    mkdir -p TRAVIS_PREFIX
    curl "$1" | tar -C TRAVIS_PREFIX -xJ
}

sudo apt-get install yasm check libopenal-dev libdbus-1-dev libv4l-dev

#installing libsodium, needed for Core

get https://build.tox.chat/view/libsodium/job/libsodium_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libsodium.tar.xz
get https://build.tox.chat/job/libvpx_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libvpx.tar.xz
get https://build.tox.chat/job/libtoxcore_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libtoxcore.tar.xz
get https://build.tox.chat/job/libfilteraudio_build_linux_x86-64_mixed_release/lastSuccessfulBuild/artifact/libfilteraudio.tar.xz
get https://build.tox.chat/job/libopus_build_linux_x86-64_static_release/lastSuccessfulBuild/artifact/libopus.tar.xz

NEW_PREFIX="$(pwd)/TRAVIS_PREFIX"
find TRAVIS_PREFIX -name "*.pc" | while read FILENAME; do
    sed -E -e 's|^prefix=.*|prefix='$NEW_PREFIX'|' -i $FILENAME
done

#find TRAVIS_PREFIX

