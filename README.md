# XAVRX-WII

Experimental Nintendo Wii emulator project for Nintendo 3DS / 2DS XL, written in C++20.

## Version

**V1 — 1.0.0-alpha.1**

XAVRX-WII uses an open-ended version sequence: V1, V2, V3, and onward. Each new version should build on the previous one with additional implementation, tests, fixes, and verified improvements. There is no predetermined final version.

## What V1 contains

- Native Nintendo 3DS homebrew application with a two-screen text interface.
- System initialization and an experimental Wii hardware model.
- Initial Broadway PowerPC CPU state, decoder, execution and control-flow foundations.
- Wii memory model and partial Hollywood / Starlet / peripheral models.
- Controller profile screens for Wii Remote, Nunchuk and Classic Controller.
- A native 3DS Balance Demo that uses the Circle Pad to simulate four load sensors.
- Automated CMake test suite and Nintendo 3DS build workflow.
- CI packaging targets for .3dsx and .cia; each artifact must be confirmed by a successful workflow run before it is called verified.

## Important limitations

V1 is an early foundation, **not a playable Wii emulator**. It does not currently boot the Wii System Menu or run commercial Wii games. The controller entries are profiles, not real Wii Bluetooth connections. The Balance Demo is a native 3DS simulation and does not read a physical Balance Board. CPU, GPU, DSP, IOS, firmware boot, disc loading, audio, and peripheral emulation are incomplete.

A successful .3dsx or .cia build proves that the homebrew application was packaged; it does not prove Wii game compatibility or playable performance on a 2DS XL.

## Build and test the desktop core

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run xavrx-wii --version to print the project version. Run xavrx-wii to initialize the experimental hardware foundation.

## Nintendo 3DS / 2DS XL

Build with devkitPro / devkitARM:

```sh
make -f 3ds/Makefile
```

The GitHub Actions workflow builds the .3dsx application and attempts to package a .cia. Check the workflow result and artifacts before installing.

## Development

See [architecture](docs/ARCHITECTURE.md), [CPU notes](docs/CPU.md), [system hardware notes](docs/SYSTEM_HARDWARE.md), and [the ongoing version roadmap](docs/ROADMAP.md). A new version is considered verified only after its relevant builds and automated tests pass, with remaining limitations clearly documented.