# XAVRX-WII Architecture

## Goal

Build a Nintendo Wii emulator from a clean, testable C++ codebase. The target console is the original Nintendo Wii; Wii U-specific hardware and vWii behavior are out of scope unless explicitly added later.

## Design principles

- C++20 and CMake, with a portable host-side build.
- Hardware subsystems remain separate and communicate through explicit interfaces.
- Correctness and deterministic tests take priority over premature optimization.
- Unsupported hardware behavior must be reported honestly rather than silently approximated.
- Game images and system files are user-provided; the project does not distribute copyrighted games or firmware.

## Planned modules

| Module | Responsibility |
| --- | --- |
| Core | Lifecycle, configuration, timing coordination, errors |
| CPU | Gekko / PowerPC instruction execution, registers, exceptions |
| Memory | Configurable RAM, big-endian aligned accesses, non-overlapping memory-mapped devices |
| System | Clock domains, deterministic timers, interrupt controller, boot-state transitions |
| GPU | Hollywood / GX command processing and rendering |
| Audio | DSP behavior, audio timing, output interface |
| Input | Wii Remote and supported controller abstractions |
| Disc | User-supplied disc image parsing and data access |
| Frontend | Window, configuration, logs, and user interaction |

## Current implementation boundary

The project has an initial CPU execution foundation and configurable memory bus. The memory layer does not yet model the complete Wii address map, MMU, device registers, DMA, or cache aliases. The emulator cannot boot games or provide playable emulation yet.
