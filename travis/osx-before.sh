#!/bin/sh

set -e -x

brew update

brew tap Tox/tox
brew install --HEAD libtoxcore
