#!/bin/sh

TAG=$(git describe --abbrev=0 --tags)
USER=$(git config --get user.name)

curl -O "https://github.com/uTox/uTox/archive/$TAG.zip" -O "https://github.com/uTox/uTox/archive/$TAG.tar.gz"

gpg --armor --detach-sign "$TAG.zip"
gpg --armor --detach-sign "$TAG.tar.gz"

mv "$TAG.zip.asc" "uTox-$TAG.$USER.zip.asc"
mv "$TAG.tar.gz.asc" "uTox-$TAG.$USER.tar.gz.asc"

rm "$TAG.zip"
rm "$TAG.tar.gz"
