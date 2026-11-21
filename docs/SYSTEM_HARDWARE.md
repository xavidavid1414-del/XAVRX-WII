# XAVRX-WII System Hardware Foundation

## Clock domains
The initial clock tree exposes CPU (729 MHz), GPU (243 MHz), bus (162 MHz), and DSP (81 MHz) reference frequencies. A caller may provide alternate non-zero frequencies for deterministic tests. Cycle-to-nanosecond conversion is provided for scheduler integration. These are model reference rates, not a claim that every Wii timing relationship or PLL mode is reproduced.

## Interrupt controller
The controller tracks 32 pending and enabled interrupt lines independently. Raising an interrupt sets its pending bit; enabling a line allows it to appear in the active mask. Claiming selects the lowest-numbered active line and acknowledges only that line. Out-of-range line operations are ignored.

## Timers
Four deterministic timers advance only when the host scheduler supplies CPU cycles. Timers support one-shot and periodic operation, configurable non-zero reload values, enable/disable, and accumulated expiration counts. Expiration raises the corresponding interrupt line (default lines 8–11). Large cycle advances account for multiple periodic expirations without per-cycle loops. Expiration counters saturate instead of wrapping.

## Boot state
The boot controller validates transitions through Off -> BootRom -> Firmware -> Running, with Halted/Running and Faulted/Off transitions. Invalid transitions leave the current state unchanged. The SystemHardware aggregate coordinates the timer bank, interrupt controller, clock tree, and boot state and provides a reset operation.

## Scope
This is a deterministic architectural foundation for later scheduler and hardware integration. It is not a cycle-accurate Wii clock/interrupt implementation and does not yet model Hollywood-specific interrupt registers, bus arbitration, PLL behavior, watchdog registers, or firmware execution.
