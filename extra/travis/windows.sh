#!/bin/sh
# Build Windows x64 µTox from source (dependencies + utox.exe).
# Run from the repository root:
#   ./extra/travis/windows.sh
set -eux

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

. ./extra/travis/env.sh
mkdir -p "$CACHE_DIR/usr"

./extra/travis/windows-before.sh
./extra/travis/windows-script.sh

echo "Built: $ROOT/build_win/utox.exe"
