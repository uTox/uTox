#!/bin/sh

get() {
    mkdir -p TRAVIS_PREFIX
    curl "$1" | tar -C TRAVIS_PREFIX -xJ
}

sudo apt-get update
sudo apt-get install yasm check libopenal-dev libdbus-1-dev libv4l-dev

#installing libsodium, needed for Core

get https://build.tox.chat/job/libsodium_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libsodium_build_windows_x86-64_static_release.zip
get https://build.tox.chat/job/libvpx_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libvpx_build_windows_x86-64_static_release.zip
get https://build.tox.chat/job/libtoxcore_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libtoxcore_build_windows_x86-64_static_release.zip
get https://build.tox.chat/job/libfilteraudio_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libfilteraudio.zip
get https://build.tox.chat/job/libopus_build_windows_x86-64_static_release/lastSuccessfulBuild/artifact/libopus_build_windows_x86-64_static_release.zip

NEW_PREFIX="$(pwd)/TRAVIS_PREFIX"
find TRAVIS_PREFIX -name "*.pc" | while read FILENAME; do
    sed -E -e 's|^prefix=.*|prefix='$NEW_PREFIX'|' -i $FILENAME
done

#find TRAVIS_PREFIX

