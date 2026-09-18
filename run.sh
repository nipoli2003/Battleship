#!/usr/bin/env bash
set -e # Stop immediately if any step fails

echo "==> Configuring..."
cmake -B build

echo "==> Building..."
cmake --build build

echo "==> Running Tests..."
ctest --test-dir build --output-on-failure

echo "==> All passed! Launching Battleship..."
./build/battleship