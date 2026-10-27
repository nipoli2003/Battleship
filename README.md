# Battleship

> [!CAUTION]
> **Contributing:** Before you make your first contribution, read [CONTRIBUTING.md](CONTRIBUTING.md).

Multiplayer, server-authoritative Battleship in C++20, with a Raylib GUI.

Two players connect to a relay server, create or join a lobby with a short code, place their fleet, and trade shots. Clients send intents, the server validates them and broadcasts a per-player view, so a client only ever knows its own board plus what it has actually fired at.

**Status:** 1v1 online play is working end to end. Single-player/AI and tactical abilities are not implemented yet (see Issues).

## Requirements
CMake 3.20+, a C++20 compiler (GCC 11+ / Clang 14+). CMake downloads all requirements (Raylib, sockpp, glaze, GoogleTest) automatically.

Linux needs the usual X11/GL/audio dev packages:
```bash
sudo apt-get install -y build-essential cmake ninja-build pkg-config \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgl1-mesa-dev libasound2-dev libssl-dev
```

macOS works out of the box, Windows is untested

## Build, Test & Run
To buils, test and launch **two clients and a server**:
```bash
./run.sh
```
Or do it manually:
```bash
# build
cmake -B build
cmake --build build
# test
ctest --test-dir build --output-on-failure
# run
./build/battleship_server
./build/battleship
./build/battleship
```

The executables are `build/battleship` and `build/battleship_server`
The server is currently hardcoded to listen to port 9000.

## How it works
- read our SRS (Software Requirement Specification):
- read our SDS (Software Design Specification):
