#!/bin/sh
set -e -u -x

. ./travis/env.sh

echo "@grayhatter, you need to add FILTER_AUDIO support back in!!"

make FILTER_AUDIO=0 DBUS=0
