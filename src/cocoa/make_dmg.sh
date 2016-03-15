#!/bin/sh
WORK=$(mktemp -d utox_dmg.XXXXXXX) || exit 1
tar -C $WORK -xjf "src/cocoa/frozen_dmg.tar.bz2"

rm $WORK/uTox/uTox.app
cp -r "uTox.app" "$WORK/uTox/uTox.app"
cp -r "utox-static" "$WORK/uTox/uTox.app/Contents/MacOS/utox"
chmod +x "$WORK/uTox/uTox.app/Contents/MacOS/utox"
if [ ".$CODE_SIGN_IDENTITY" = "." ]; then
    CODE_SIGN_IDENTITY="-"
fi
codesign -s "$CODE_SIGN_IDENTITY" -fv "$WORK/uTox/uTox.app/Contents/MacOS/utox"
touch $WORK/uTox/.Trash

hdiutil create -megabytes 32 -srcfolder $WORK/uTox \
        -format UDBZ -nospotlight -noanyowners "uTox.dmg"
rm -rf $WORK
