#!/bin/sh
TMPFILE=$(mktemp .utox_info_plist.XXXXXXX)
cp $1 $TMPFILE

plist_set() {
    /usr/libexec/PlistBuddy -c "Set :$1 '$2'" $TMPFILE
}

plist_set CFBundleIdentifier "future.utox"
plist_set CFBundleExecutable "utox"
plist_set LSMinimumSystemVersion "10.7"
plist_set CFBundleName "uTox"

mv $TMPFILE "utox-Info.plist"