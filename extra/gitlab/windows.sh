#!/bin/sh
# Build Windows x64 µTox from source (dependencies + utox.exe).
# Run from the repository root:
#   ./extra/gitlab/windows.sh
set -eux

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

export GL_BUILD="${GL_BUILD:-win64}"
. ./extra/gitlab/env.sh
mkdir -p "$CACHE_DIR/usr"

./extra/gitlab/windows-before.sh
./extra/gitlab/windows-script.sh

echo "Built: $ROOT/build_win/utox.exe"
