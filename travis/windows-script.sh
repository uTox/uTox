#!/bin/sh
set -eux

. ./travis/env.sh

make FILTER_AUDIO=0 DBUS=0
