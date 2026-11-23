# XAVRX-WII — Ongoing Version Roadmap

XAVRX-WII follows an open-ended sequence of versions: V1, V2, V3, and onward. There is no fixed number of versions or predetermined final release. Every version should preserve working features, add a clearly scoped improvement, run relevant tests, fix regressions, and document what remains incomplete.

## V1 — 1.0.0-alpha.1

Initial Nintendo 3DS / 2DS XL homebrew foundation.

- C++20 project structure, CMake build, and automated test suite.
- Nintendo 3DS homebrew target with a two-screen interface.
- Initial Wii memory, Hollywood / Starlet, peripheral, and Broadway CPU models.
- Controller profile screens and a native 3DS Balance Demo.
- CI workflow targets for .3dsx and .cia packaging.

**V1 status:** baseline source and packaging workflow are present. The latest build results must be checked before declaring each artifact verified. This is not a playable Wii emulator.

## Next development targets

These are continuing engineering priorities, not a finite phase count. Their order may change as tests expose dependencies.

1. Finish and test the Wii hardware foundation: memory/MMIO behavior, clocks, reset sequencing, interrupt routing, timers, NAND, SD, USB, DI, WLAN, Bluetooth, and device timing.
2. Expand Broadway PowerPC instruction coverage, exceptions, memory behavior, and CPU tests.
3. Implement the Starlet ARM processor and the IOS-facing system interfaces needed for boot.
4. Build the Hollywood GX command pipeline, graphics state, texture handling, and framebuffer output.
5. Implement DSP task execution, audio processing, and synchronized output.
6. Add Wii Remote, Nunchuk, Classic Controller, and other input paths with honest hardware capability reporting.
7. Add safe read-only parsing for user-provided Wii disc images, followed by executable loading and boot integration.
8. Improve deterministic timing, subsystem integration, compatibility tests, diagnostics, and performance.
9. Verify 3DSX and CIA builds independently and test on real supported hardware where possible.

## Release truthfulness

- A successful C++ build or unit test does not prove Wii game compatibility.
- A successful .3dsx or .cia packaging job proves only that the homebrew application was packaged.
- Controller profiles are not Bluetooth connectivity unless a real transport and protocol implementation are present.
- Simulated sensor values are not physical Balance Board measurements.
- No version may claim to boot or run a Wii game until that behavior has been demonstrated by reproducible tests.

Full-speed Wii emulation on Nintendo 2DS XL is a major technical challenge and is not guaranteed. Each release must state exactly what is implemented and what remains experimental.