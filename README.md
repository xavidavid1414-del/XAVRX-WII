# XAVRX-WII

An experimental Nintendo Wii emulator research project written in C++.

> **Status:** early development. This repository is a foundation, not a playable emulator. It does not currently boot Wii software or run commercial games.

## Current foundation

- C++20 project structure with CMake
- Initial byte-addressable memory component
- Big-endian 8-, 16-, and 32-bit memory access
- Bounds checking for memory accesses
- Automated GitHub Actions build and test workflow

## Build locally

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Development roadmap

1. Memory map and hardware address regions
2. PowerPC 750CL (Broadway) CPU interpreter
3. Exceptions, interrupts, and timing
4. Wii system and device interfaces
5. Flipper/Hollywood graphics and audio subsystems
6. Wii Remote input
7. Boot process, homebrew compatibility, and later game compatibility

Accuracy and tested behavior take priority over claiming compatibility prematurely.
