#!/bin/sh
set -eu
cd "$(dirname "$0")"
BUILD_DIR="${TMPDIR:-/tmp}/freeink-opds-host-tests"
mkdir -p "$BUILD_DIR"
c++ -std=c++17 -Wall -Wextra ${CXXFLAGS:-} -fsanitize=address,undefined \
  -Istubs -I../../include -I../../../JsonSax/include \
  ../../src/*.cpp ../../../JsonSax/src/StreamingJsonParser.cpp \
  test_client.cpp -lexpat -o "$BUILD_DIR/test_client"
"$BUILD_DIR/test_client"
c++ -std=c++17 -Wall -Wextra -Werror -I../../include test_entries_fitting.cpp -o "$BUILD_DIR/test_entries_fitting"
"$BUILD_DIR/test_entries_fitting"
for board in '' -DFREEINK_DEVICE_X4PRO=1; do
  c++ -std=c++17 -Wall -Wextra ${CXXFLAGS:-} $board -fsanitize=address,undefined \
    -Istubs -I../../include -I../../../JsonSax/include \
    ../../src/*.cpp ../../../JsonSax/src/StreamingJsonParser.cpp \
    test_description.cpp -lexpat -o "$BUILD_DIR/test_description"
  "$BUILD_DIR/test_description"
done
