# Phase 6 — Controller batch 2

## Classic Controller Pro
- Separate controller profile from the standard Classic Controller.
- 15 digital controls and two normalized analog sticks.
- L/R shoulders are represented as digital buttons; no analog trigger values are exposed by this Pro profile.
- Input values are clamped to [-1, 1], invalid button values are rejected, and reset restores neutral state.

## Wii Balance Board
- Four independently calibrated load-sensor inputs: top-left, top-right, bottom-left, bottom-right.
- Each corner converts raw sensor units to kilograms with its own offset and scale.
- Total weight is the sum of the four nonnegative corner loads.
- Center of pressure is calculated from the four corner loads in normalized board coordinates [-1, 1].
- Tare records current raw readings as the unloaded baseline.
- Non-finite and negative raw readings are sanitized; invalid calibration is rejected.

## Integration and limitations
Both controllers are exposed through WiiSystem and are reset by WiiSystem::reset().
The Balance Board API accepts sensor readings supplied by an input backend. This commit does not implement Bluetooth pairing, the real board's report protocol, physical sensor electronics, or hardware access from a Nintendo 2DS XL. Without a real board and a compatible input bridge, readings must be supplied by a simulator or test harness. The kilograms shown by this abstraction are only physically meaningful after calibration against known reference loads.
