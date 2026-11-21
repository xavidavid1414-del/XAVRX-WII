# XAVRX-WII
Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status
**Current phase: 2 / 20 — CPU foundation complete.**

The project now has a Gekko architectural register/state model and an instruction identity decoder for an initial documented subset of PowerPC encodings. This is not yet a playable emulator: instructions are not executed and Wii games cannot boot.

## Build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Run xavrx-wii --version to print the project version.

## Development
See [the architecture document](docs/ARCHITECTURE.md), [CPU foundation notes](docs/CPU.md), and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and repeat verification.
