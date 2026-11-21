# XAVRX-WII
Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status
**Current phase: 4 / 20 — CPU control flow and exception foundation implemented.**

The CPU has an architectural state model, a decode layer, and a tested initial integer executor supporting ADDI, ADDIS, ORI, ORIS, XORI, XORIS, ANDI., ADD, SUBF, AND, OR, XOR, and signed register CMP. This is not yet a playable emulator: graphics, memory, boot integration, peripherals, and many CPU instructions remain unimplemented.

## Build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Run xavrx-wii --version to print the project version.

## Development
See [architecture](docs/ARCHITECTURE.md), [CPU notes](docs/CPU.md), and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and repeat verification.
