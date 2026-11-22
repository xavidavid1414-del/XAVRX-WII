# Phase 6 — Controller input foundations (batch 1)

This batch adds three independent, deterministic input-state models:
- Wii Remote: 11 digital buttons, normalized pointer visibility/position, three-axis acceleration, and Nunchuk attachment state.
- Nunchuk: C/Z buttons, two-axis analog stick, and three-axis acceleration.
- Classic Controller: 15 digital buttons, two analog sticks, and two analog trigger values.

All analog values are sanitized: stick/acceleration values clamp to [-1, 1], pointer coordinates to [0, 1], and triggers to [0, 1]. Non-finite floating-point input is replaced with a neutral/default value. Each controller can be reset independently and is exposed through WiiSystem.

Scope note: these are complete host-side input-state abstractions for the listed controls, not yet a complete Wii Bluetooth HID/extension wire protocol, sensor calibration implementation, or game compatibility layer. Physical motion is only available when a host input source supplies it. Classic Controller Pro shares the same control layout and will be handled in the next batch as its own profile.
