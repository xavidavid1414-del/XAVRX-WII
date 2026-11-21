# XAVRX-WII

Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status

**Current phase: 1 / 20 — Project foundation.**

The repository currently contains the initial architecture, portable build scaffold, command-line entry point, smoke tests, and GitHub Actions workflow. This is not yet a playable emulator and cannot boot Wii games.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run `xavrx-wii --version` to print the project version.

## Development

See [the architecture document](docs/ARCHITECTURE.md) and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and verification.
