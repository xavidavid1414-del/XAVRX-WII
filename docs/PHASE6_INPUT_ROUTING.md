# Phase 6 — Controller input routing

This batch adds a shared mapping layer between the five controller state models and game-facing actions.

- Converts Wii Remote, Nunchuk, Classic Controller, Classic Controller Pro, and Balance Board states into normalized input frames.
- Supports configurable digital and analog bindings, per-binding scale, and analog deadzones.
- Keeps controller profiles separate so identically named controls on different devices cannot collide.
- Exposes Balance Board total load as a normalized game input; raw kilograms remain available from the Balance Board state model.
- Invalid profile/control/action values and invalid deadzones are rejected. Non-finite input values are sanitized.

The repository currently has no settings-menu/UI framework, so this batch provides the reusable mapping API and tests rather than inventing a UI. A later UI layer can edit the bindings through `ControllerInputRouter::add_binding`.

This is a software input abstraction, not Bluetooth HID support or a guarantee of compatibility with every Wii title. Balance Board readings still require real sensor hardware and calibration.
