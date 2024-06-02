#!/usr/bin/env bash


set -o errexit
set -o nounset
set -o pipefail

if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

if [[ "${1-}" =~ ^-*h(elp)?$ ]]; then
    echo 'Usage: ./rebuild_and_test.sh

    Quick script to rebuild the project with unit tests and run tests.

'
    exit
fi

rebuild() {
  local ROOT_REAL="$(dirname $(realpath "$0"))/.."
  local BUILD_DIR="$ROOT_REAL/build"
  echo "changing to $ROOT_REAL" 
  cd "$ROOT_REAL"
  echo "removing $BUILD_DIR"
  rm -rf "$BUILD_DIR"

  echo "Building in $BUILD_DIR"
  mkdir "$BUILD_DIR" && cd "$BUILD_DIR" && cmake .. -DWITH_UNIT_TESTING=ON && make
  echo "changing to $BUILD_DIR"
  cd "$BUILD_DIR"
  ctest --verbose
}

rebuild
