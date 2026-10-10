#!/bin/sh
# Build Windows x64 µTox from source (dependencies + utox.exe).
# Run from the repository root:
#   ./extra/ci/windows.sh
set -eux

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

. ./extra/ci/env.sh
mkdir -p "$CACHE_DIR/usr"

./extra/ci/windows-before.sh
./extra/ci/windows-script.sh

echo "Built: $ROOT/build_win/utox.exe"
