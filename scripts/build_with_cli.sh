#!/usr/bin/env bash


set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

# TODO make `WITH_DEBUG` option a flag that is pasted in so this script can build both debug and release builds.
# TODO make this a generic "build" script and have an option for building a library vs executable with the cli.
if [[ "${1-}" =~ ^-*h(elp)?$ ]]; then
    echo 'Usage: ./build_with_cli.sh

    Quick script to rebuild the project with the cli and no unit tests but with debugging enabled.'
    exit
fi

build() {
  local ROOT_REAL="$(dirname $(realpath "$0"))/.."
  local BUILD_DIR="$ROOT_REAL/build"
  echo "changing to $ROOT_REAL" 
  cd "$ROOT_REAL"
  echo "removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"

  echo "Building in $BUILD_DIR"
  #mkdir "$BUILD_DIR" && cd "$BUILD_DIR" && cmake .. -DWITH_DEBUG=ON && make
  mkdir "$BUILD_DIR" && cd "$BUILD_DIR" && cmake .. && make
  echo "changing to $BUILD_DIR"
  cd "$BUILD_DIR"
  ./geoqlite
}

build
