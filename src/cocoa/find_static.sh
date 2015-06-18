#!/bin/bash
# Find .a archives for static linking purposes
# Copyright 2015 Zodiac Labs but you are free
# to reuse anywhere without permission

unmangle_ldflags() {
    for LIBWHAT in $@; do
        printf "$(echo $LIBWHAT | sed -e s/-l/lib/g -e s/\$/.a/g) "
    done
}

LDFLAGS=
for LIBRARY in $@; do
    SP=$(pkg-config --libs-only-L $LIBRARY | sed s/-L//g)
    NLIB=$(unmangle_ldflags $(pkg-config --libs-only-l $LIBRARY))
    for ARCHIVE in $NLIB; do
        LDFLAGS="$LDFLAGS $(find $SP -name $ARCHIVE | head -n 1)"
    done
done

echo $LDFLAGS | sort | uniq
