# XAVRX-WII
Nintendo Wii emulator project built from scratch in C++20 with CMake.

## Status
**Current phase: 6 / 20 — Wii hardware foundation integrated; phase remains in progress.** The initialized system now composes 24 MiB MEM1, 64 MiB MEM2, shared CPU-aware memory/MMIO dispatch, Hollywood dual-CPU IRQs, IPC handshakes, shared GPIO state, and the Starlet timer/alarm. The current app can initialize and reset this hardware foundation. This is not yet a playable emulator: ARM Starlet execution, complete Gekko execution, MMU/cache behavior, firmware boot, watchdog/reset/PLL controls, peripherals, graphics, and audio remain incomplete.

## Build
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```
Run xavrx-wii --version to print the project version. Run xavrx-wii to initialize the hardware foundation.

## Development
See [architecture](docs/ARCHITECTURE.md), [CPU notes](docs/CPU.md), [system hardware notes](docs/SYSTEM_HARDWARE.md), and [the 20-phase roadmap](docs/ROADMAP.md). A phase advances only after implementation, testing, corrections, and repeat verification.
