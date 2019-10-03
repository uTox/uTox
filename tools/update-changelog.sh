#!/bin/sh

TOKEN=$(git config --get user.token)

if [ -z "$TOKEN" ]; then
    echo "Please add your github token to user.token"
    echo "Run git config --local user.token [token]"
    exit 1
fi

github_changelog_generator -u uTox -t "$TOKEN"
