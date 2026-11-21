# XAVRX-WII
Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status
**Current phase: 6 / 20 — Wii-specific hardware expansion in progress — MEM1/MEM2, Hollywood IRQ, IPC, GPIO, and MMIO foundations implemented.**

The CPU has an architectural state model and an initial integer/control-flow executor. The memory subsystem includes a big-endian bus with memory-mapped devices, plus Wii MEM1 (24 MiB) / MEM2 (64 MiB) with cached, uncached, and physical aliases. Hollywood's documented IRQ flag/mask pairs, IPC handshake registers, GPIO banks, and MMIO windows have initial tested models. This is not yet a playable emulator: the complete Wii memory map, MMU/cache behavior, many hardware devices, graphics, audio, boot integration, peripherals, and many CPU instructions remain unimplemented.

## Build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Run xavrx-wii --version to print the project version.

## Development
See [architecture](docs/ARCHITECTURE.md), [CPU notes](docs/CPU.md), [system hardware notes](docs/SYSTEM_HARDWARE.md), and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and repeat verification.
