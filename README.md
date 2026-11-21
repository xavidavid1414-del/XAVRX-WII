# XAVRX-WII
Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status
**Current phase: 6 / 20 — Wii-specific hardware expansion in progress.** Current foundations include MEM1/MEM2 aliases, Hollywood dual-CPU IRQs, IPC handshakes, shared GPIO state, CPU-specific MMIO views, and the Starlet timer/alarm. The CPU has an architectural state model and an initial integer/control-flow executor. This is not yet a playable emulator: complete memory/MMU/cache behavior, many hardware devices, graphics, audio, boot integration, peripherals, and many CPU instructions remain unimplemented.

## Build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Run xavrx-wii --version to print the project version.

## Development
See [architecture](docs/ARCHITECTURE.md), [CPU notes](docs/CPU.md), [system hardware notes](docs/SYSTEM_HARDWARE.md), and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and repeat verification.
