#!/bin/sh

export CACHE_DIR=`pwd`/cache/$GL_BUILD
mkdir -p $CACHE_DIR || true

. ./extra/common/env.sh

export CLICOLOR_FORCE=1
