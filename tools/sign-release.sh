#!/bin/sh
# set -e
TAG=$(git describe --abbrev=0 --tags)
USER=${USER:-$(git config --get user.name)}
VERSION=${TAG/v/}

echo "Going to verify and sign releases."
echo ""
echo ""

git diff --exit-code >> /dev/null || (echo "Working Dir not clean, this script won't work." && false)

echo ""
echo "Getting $TAG.zip"
curl -LOs "https://github.com/uTox/uTox/archive/$TAG.zip"
unzip -q "$TAG.zip"
cp -r "uTox-$VERSION"/* .
rm -r "uTox-$VERSION"

echo "Checking $TAG.zip"
if git diff --exit-code >> /dev/null; then
    echo "PASSED $TAG.zip"
    gpg --armor --detach-sign "$TAG.zip"
    mv "$TAG.zip.asc" "uTox-$TAG.$USER.zip.asc"
    rm "$TAG.zip"
else
    echo "FAILED $TAG.zip"
    rm "$TAG.zip"
fi


echo ""
echo "Getting $TAG.tar.gz"
curl -LOs "https://github.com/uTox/uTox/archive/$TAG.tar.gz"
tar xf "$TAG.tar.gz"
cp -r "uTox-$VERSION"/* .
rm -r "uTox-$VERSION"

echo "Checking $TAG.tar.gz"
if git diff --exit-code >> /dev/null; then
    echo "PASSED $TAG.tar.gz"
    gpg --armor --detach-sign "$TAG.tar.gz"
    mv "$TAG.tar.gz.asc" "uTox-$TAG.$USER.tar.gz.asc"
    rm "$TAG.tar.gz"
else
    echo "FAILED $TAG.tar.gz"
    rm "$TAG.tar.gz"
fi

