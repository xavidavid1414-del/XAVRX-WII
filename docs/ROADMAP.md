# XAVRX-WII — 20-Phase Roadmap
A phase is complete only after implementation, automated checks, correction of discovered defects, and repeat verification. No phase is skipped.

1. **Project foundation** — COMPLETE. Repository structure, C++20/CMake, architecture boundaries, smoke tests, CI.
2. **CPU foundation** — COMPLETE. Gekko register model, CPU architectural state, condition-register access, and an initial instruction decode framework with focused tests.
3. **PowerPC integer execution** — COMPLETE. Implemented and tested ADDI/ADDIS, immediate logical operations, ADD/SUBF, AND/OR/XOR, signed register CMP, CR updates, PC advancement, and safe rejection of unsupported encodings.
4. **CPU control flow and exceptions** — branches, traps, interrupts, and exception state.
5. **Memory subsystem** — address space, access widths, alignment, and memory-mapped I/O contracts.
6. **System hardware model** — clocks, timers, interrupt controller, and boot-state model.
7. **Boot and firmware interfaces** — documented boot flow and replaceable firmware/system interfaces.
8. **Hollywood/GX command foundation** — command processor and validated command stream.
9. **Graphics pipeline** — GX state model and primitive processing.
10. **Texture and framebuffer support** — formats, sampling, framebuffer operations.
11. **Audio DSP foundation** — DSP state, task interface, and timing model.
12. **Audio output** — mixing, buffering, and host audio abstraction.
13. **Wii Remote input** — buttons, pointer data, and sensor abstraction.
14. **Controller support** — Nunchuk, Classic Controller, and GameCube controller abstractions.
15. **Disc image support** — read-only parsing and access to user-supplied Wii disc images.
16. **Executable loading and boot integration** — connect loader, memory, CPU, and system startup.
17. **Timing and synchronization** — coordinate CPU, GPU, audio, and input deterministically.
18. **Compatibility and regression suite** — reproducible subsystem tests and compatibility reporting.
19. **Performance and stability** — profiling-led optimization, diagnostics, and robustness fixes.
20. **Release preparation** — packaging, documentation, supported-feature matrix, and final verification.

The roadmap is an engineering sequence, not a promise that full-speed or full-game compatibility is guaranteed. Wii emulation is a large, long-term undertaking.
