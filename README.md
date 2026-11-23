# XAVRX-WII

Experimental Nintendo Wii emulator project for Nintendo 3DS / 2DS XL, written in C++20.

## Version

**V1 — 1.0.0-alpha.4**

The project uses an open-ended version sequence: alpha.3, alpha.4, and onward. Each release must add verified implementation and tests.

## Alpha.4 changes

- Adds a lower-screen mapping reference for Wii Remote, Nunchuk, and Classic Controller using the physical Nintendo 2DS XL controls.
- Shows the Circle Pad, C-Stick, D-Pad, face buttons, shoulder buttons, START, SELECT, and HOME mappings.
- These are default mapping previews only; live input remapping is not yet connected.

## Alpha.3 changes\n\n- Adds a dedicated Controllers screen accessible from the upper-screen main menu.\n- Adds D-pad navigation between Wii Remote, Nunchuk, Classic Controller, Controller Profiles, and Calibrate Controls.\n- Shows the relevant control-option names on the lower screen.\n- Keeps unsupported Bluetooth pairing, profile storage, calibration, and controller mappings explicitly marked as not implemented.\n\n## Alpha.2 changes

- English dual-screen home menu with the complete main menu on the upper screen.
- Touch target in the upper-right of the lower screen opens Settings.
- Settings categories: Controls, Interface, Audio, Display, Performance, Other Settings.
- Controls page lists Wii Remote, Nunchuk, Classic Controller, controller profiles, and calibration.
- Load Game entry is present as a clearly marked placeholder.

## Current implementation status

The settings interface is a navigation shell. Its entries do not yet change real controller mappings, audio, display, or performance settings. Wii MotionPlus and Balance Board hardware support are not implemented. The current controls are not connected to real Wii Bluetooth devices.

The Broadway CPU and Wii hardware models remain partial. **There is no complete Wii ISO/WBFS loader, no Wii boot process, and no verified Wii game execution in alpha.4.** The Load Game screen explicitly reports this limitation; it does not pretend to load a game.

The existing Balance Demo from the earlier build is a native 3DS simulation, not a Wii title. A successful .3dsx or .cia build proves packaging only, not Wii game compatibility or playable performance on a 2DS XL.

## Build and test the desktop core

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Nintendo 3DS / 2DS XL

Build with devkitPro / devkitARM:

```sh
make -f 3ds/Makefile
```

Check the GitHub Actions run and inspect both generated packages before installing.
