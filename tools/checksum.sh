#!/bin/sh

TAG=$(git describe --abbrev=0 --tags)

curl -LOs "https://github.com/uTox/uTox/archive/$TAG.zip"
curl -LOs "https://github.com/uTox/uTox/archive/$TAG.tar.gz"

echo
echo "md5"
echo "-----------------------------------------"
md5sum "$TAG.zip"
md5sum "$TAG.tar.gz"
echo "-----------------------------------------"

echo
echo "sha256"
echo "-----------------------------------------"
sha256sum "$TAG.zip"
sha256sum "$TAG.tar.gz"
echo "-----------------------------------------"

rm "$TAG.zip"
rm "$TAG.tar.gz"
