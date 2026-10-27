#!/usr/bin/env bash
set -e

cmake --build build
# ctest --test-dir build --output-on-failure
./build/battleship & ./build/battleship & ./build/battleship_server
