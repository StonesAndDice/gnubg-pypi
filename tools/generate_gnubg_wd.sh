#!/usr/bin/env bash
# Generate src/gnubgmodule/data/gnubg.wd for use when the build environment
# does not provide a shared libpython (e.g. manylinux/cibuildwheel).
# Run this on a system where makeweights can be built (Linux/macOS with
# python3-dev and shared libpython). Then commit the generated file.
set -e
cd "$(dirname "$0")/.."
weights=src/gnubgmodule/data/gnubg.weights
out=src/gnubgmodule/data/gnubg.wd
if [[ ! -f "$weights" ]]; then
  echo "Missing $weights" >&2
  exit 1
fi
meson setup build -Dbuildtype=release
meson compile -C build
./build/makeweights -f "$out" "$weights"
echo "Generated $out"
