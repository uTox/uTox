#!/bin/bash

if [[ -z $1 ]]; then
    echo "A token is required."
    exit 1
fi

if [[ $(which github_changelog_generator) < /dev/null ]]; then
    echo "github_changelog_generator can not be found."
    exit 1
fi

github_changelog_generator -u uTox -t "$1"
