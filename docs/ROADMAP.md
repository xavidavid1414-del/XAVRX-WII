# XAVRX-WII — 20-Phase Roadmap
A phase is complete only after implementation, automated checks, correction of discovered defects, and repeat verification. No phase is skipped.

1. **Project foundation** — COMPLETE. Repository structure, C++20/CMake, architecture boundaries, smoke tests, CI.
2. **CPU foundation** — COMPLETE. Gekko register model, CPU architectural state, condition-register access, and an initial instruction decode framework with focused tests.
3. **PowerPC integer execution** — COMPLETE. Implemented and tested ADDI/ADDIS, immediate logical operations, ADD/SUBF, AND/OR/XOR, signed register CMP, CR updates, PC advancement, and safe rejection of unsupported encodings.
4. **CPU control flow and exceptions** — COMPLETE. Implemented relative/absolute and conditional branches, LR/CTR indirect branches, TWI/TW traps, system-call/program/external exception entry, interrupt masking, SRR save/restore, and RFI with automated tests.
5. **Memory subsystem** — COMPLETE. Added configurable 32-bit RAM range, big-endian 8/16/32/64-bit accessors, natural-alignment and bounds checks, explicit access results, non-overlapping memory-mapped device regions, and callback error propagation with automated tests.
6. **System hardware model** — IN PROGRESS (expanded Wii scope). Implemented reference clock domains, generic timers/boot state, MEM1/MEM2 aliases, unified CPU-aware memory/MMIO dispatch, Hollywood dual-CPU IRQ and Starlet FIQ mask registers, CPU-owned IPC flags and IRQ routing, shared 24-pin GPIO with ownership and level-sensitive latched interrupts, Starlet timer/alarm IRQ 0, initial clock/reset register state, PPCKERN gating, per-engine PPC/IOP AHB permission decoding, SRNPROT SRAM-master gates for Broadway/USB1/flash-NAND/SHA/AES, composed WiiSystem initialization, and Starlet SRAM/BOOT0 mapping with mirrors and externally supplied ROM. Initial isolated scaffolding now exists for NAND, SDHC, EHCI, DI, WLAN SDIO, and Bluetooth HCI packet transport. Added an active-low Hollywood reset-line decoder with limited DSP/GPU reset side effects, a deterministic opaque-task DSP scheduler, a bounded GX command FIFO, Broadway DSP mailbox/PI/GX MMIO windows, and a first Broadway CPU fetch/execute path with LWZ/STW and cycle advancement; these are foundations, not functional DSP/GPU emulation. Still missing operational NAND/SD/USB/Wi-Fi/Bluetooth/disc controllers, DSP instruction/task execution, GX command processing and rendering, full PLL programming, complete peripheral reset sequencing, complete CPU/Starlet integration (a first Broadway fetch/step path now exists for the implemented instruction subset), and device timing. A watchdog has not been invented because no reliable watchdog register is established in the hardware register documentation used. Phase 7 remains blocked.
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
