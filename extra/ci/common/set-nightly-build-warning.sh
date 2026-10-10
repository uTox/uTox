#!/bin/sh

WARNING='NIGHTLY BUILD FOR TESTING PURPOSES ONLY'
LANG_REGEX='msgid\(UTOX_SETTINGS\)'

replace_in_branding() {
    sed -i "s/#define TITLE \"uTox/& ${WARNING}/" $1
}

replace_in_lang() {
    need_replace=0
    while read str; do
        if [ $need_replace -eq 1 ]; then
            # everything BEFORE second double quote
            start=$(echo $str | awk -F\" '{printf "%s\"%s", $1, $2}')

            # everything AFTER second double quote
            end=$(echo $str | awk -F\" -vOFS='\"' '{ $1 = $2 = ""; print }' | cut -c 2- )

            # insert warning after translation string
            sed -i "s|${str}|${start} ${WARNING}${end}|" $1
        fi
        if [[ $str =~ $LANG_REGEX ]]; then
            need_replace=1
        else
            need_replace=0
        fi
    done < $1
}

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"

# replace in lang files
for lang in $SCRIPTPATH/../../langs/*; do
    replace_in_lang $lang
done

# replace in branding files
replace_in_branding $(realpath $SCRIPTPATH/../../src/branding.h)
replace_in_branding $(realpath $SCRIPTPATH/../../src/branding.h.in)
