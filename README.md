# XAVRX-WII
Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status
**Current phase: 5 / 20 — initial memory bus implemented and verified.**

The CPU has an architectural state model, an initial integer/control-flow executor, and a configurable big-endian memory bus with aligned 8/16/32/64-bit access and memory-mapped device callbacks. This is not yet a playable emulator: the full Wii memory map, MMU, graphics, boot integration, peripherals, and many CPU instructions remain unimplemented.

## Build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Run xavrx-wii --version to print the project version.

## Development
See [architecture](docs/ARCHITECTURE.md), [CPU notes](docs/CPU.md), and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and repeat verification.
