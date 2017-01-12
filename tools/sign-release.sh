#!/bin/sh

TAG=$(git describe --abbrev=0 --tags)
USER=$(git config --get user.name)
VERSION=${TAG/v/}

wget "https://github.com/uTox/uTox/archive/$TAG.zip"
wget "https://github.com/uTox/uTox/archive/$TAG.tar.gz"

gpg --armor --detach-sign "$TAG.zip"
gpg --armor --detach-sign "$TAG.tar.gz"

mv "$TAG.zip.asc" "uTox-$TAG.$USER.zip.asc"
mv "$TAG.tar.gz.asc" "uTox-$TAG.$USER.tar.gz.asc"

unzip -q "$TAG.zip"
cp -r "uTox-$VERSION"/* .
rm -r "uTox-$VERSION"

tar xf "$TAG.tar.gz"
cp -r "uTox-$VERSION"/* .
rm -r "uTox-$VERSION"

rm "$TAG.zip"
rm "$TAG.tar.gz"

git diff --exit-code
