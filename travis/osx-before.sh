#!/bin/sh

set -eux

brew update

brew tap Tox/tox
brew install --HEAD libtoxcore
