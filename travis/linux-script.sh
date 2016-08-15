#!/bin/sh
set -e -x

PKG_CONFIG_PATH="./TRAVIS_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH" CFLAGS="-I/usr/include/freetype2 -I./TRAVIS_PREFIX/include/" make DBUS=0
