# XAVRX-WII System Hardware Foundation

## Clock domains
The initial clock tree exposes CPU (729 MHz), GPU (243 MHz), bus (162 MHz), and DSP (121.5 MHz), Starlet ARM (243 MHz) reference frequencies. A caller may provide alternate non-zero frequencies for deterministic tests. Cycle-to-nanosecond conversion is provided for scheduler integration. These are model reference rates, not a claim that every Wii timing relationship or PLL mode is reproduced.

## Interrupt controller
The controller tracks 32 pending and enabled interrupt lines independently. Raising an interrupt sets its pending bit; enabling a line allows it to appear in the active mask. Claiming selects the lowest-numbered active line and acknowledges only that line. Out-of-range line operations are ignored.

## Timers
Four deterministic timers advance only when the host scheduler supplies CPU cycles. Timers support one-shot and periodic operation, configurable non-zero reload values, enable/disable, and accumulated expiration counts. Expiration raises the corresponding interrupt line (default lines 8–11). Large cycle advances account for multiple periodic expirations without per-cycle loops. Expiration counters saturate instead of wrapping.

## Boot state
The boot controller validates transitions through Off -> BootRom -> Firmware -> Running, with Halted/Running and Faulted/Off transitions. Invalid transitions leave the current state unchanged. The SystemHardware aggregate coordinates the timer bank, interrupt controller, clock tree, and boot state and provides a reset operation.

## Scope
This is a deterministic architectural foundation for later scheduler and hardware integration. It is not a cycle-accurate Wii clock/interrupt implementation and does not yet model Hollywood-specific interrupt registers, bus arbitration, PLL behavior, watchdog registers, or firmware execution.

## Wii main-memory topology

The Wii-specific memory object models 24 MiB of MEM1 and 64 MiB of MEM2 with shared backing storage across the documented CPU cached and uncached aliases and the physical address view used by hardware-side components.

| Bank | Cached CPU view | Uncached CPU view | Physical view | Capacity |
| --- | --- | --- | --- | --- |
| MEM1 | 0x80000000 | 0xC0000000 | 0x00000000 | 24 MiB |
| MEM2 | 0x90000000 | 0xD0000000 | 0x10000000 | 64 MiB |

The implementation enforces access widths, natural alignment, bank boundaries, and big-endian reads/writes. Alias consistency, boundary behavior, and reset are covered by a dedicated test executable. This is a memory-bank foundation, not yet the complete Wii memory map: MMU/BAT translation, locked cache, MEM2 reservation/protection, SRAM, and MMIO devices remain future work.
