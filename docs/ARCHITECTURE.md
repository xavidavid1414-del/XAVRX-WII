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
| Memory | Address space, mappings, access rules, memory-mapped devices |
| System | Boot flow, interrupts, timers, hardware registers |
| GPU | Hollywood / GX command processing and rendering |
| Audio | DSP behavior, audio timing, output interface |
| Input | Wii Remote and supported controller abstractions |
| Disc | User-supplied disc image parsing and data access |
| Frontend | Window, configuration, logs, and user interaction |

## Current implementation boundary

Phase 1 only establishes the build, a minimal core API, executable entry point, smoke tests, and CI. It does not execute Wii instructions, boot games, render graphics, or provide playable emulation.
