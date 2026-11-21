# XAVRX-WII System Hardware Foundation

## Clock domains
The initial clock tree exposes CPU (729 MHz), GPU (243 MHz), bus (162 MHz), DSP (121.5 MHz), and Starlet ARM (243 MHz) reference frequencies. A caller may provide alternate non-zero frequencies for deterministic tests. These are model reference rates, not a claim that every PLL mode or timing relationship is reproduced.

## Generic timers and boot state
Four deterministic timers advance only when the host scheduler supplies CPU cycles. They support one-shot and periodic operation and raise generic timer interrupt lines. The boot controller validates Off -> BootRom -> Firmware -> Running transitions, plus halted and faulted states. These are scaffolding models, not the Wii's complete timer/alarm or boot ROM implementation.

## Wii MEM1 and MEM2
The Wii memory object models 24 MiB of MEM1 and 64 MiB of MEM2 with shared backing across cached, uncached, and physical aliases.

| Bank | Cached CPU view | Uncached CPU view | Physical view | Capacity |
| --- | --- | --- | --- | --- |
| MEM1 | 0x80000000 | 0xC0000000 | 0x00000000 | 24 MiB |
| MEM2 | 0x90000000 | 0xD0000000 | 0x10000000 | 64 MiB |

Access widths, natural alignment, bank boundaries, and big-endian reads/writes are checked. This does not yet implement MMU/BAT translation, cache behavior, SRAM, MEM2 reservation/protection, or the complete memory map.

## Hollywood interrupt controller
The model implements the two independent 32-bit Broadway and Starlet IRQ flag/mask pairs at offsets 0x30, 0x34, 0x38, and 0x3C. Hardware sources can be routed to Broadway, Starlet, or both. Flags are write-one-to-clear; masks gate each processor's asserted interrupt independently. The currently modeled source assignments include GPIO Broadway (10), GPIO Starlet (11), IPC Broadway (30), and IPC Starlet (31). Other hardware IRQ sources still need their own devices and event sources.

## Hollywood IPC
The four documented 32-bit registers are modeled at offsets 0x00–0x0C: PPCMSG, PPCCTRL, ARMMSG, and ARMCTRL. Message words are shared. X1/X2 are Broadway-owned handshake flags, Y1/Y2 are Starlet-owned flags; the other side can observe and clear the appropriate flags. IRQ enables and IRQ 30/31 signaling are modeled. ARMCTRL is Starlet-only. The implementation models the documented IOS handshake flags, not IOS itself, request queues, DMA, or the full Starlet software environment.

## Hollywood GPIO
The 24-pin GPIO register banks are modeled at offsets 0xC0–0xFC. Broadway has its GPIOB bank; Starlet has the full GPIO bank, enable and owner registers. The owner mask selects pins controlled through Broadway's bank. Inputs can be driven by the host/device layer; output, direction, straps, interrupt level, flag, and mask registers are represented. Level-sensitive matching sets the corresponding GPIO flags and raises IRQ 10 (Broadway bank) or IRQ 11 (Starlet bank). Device-specific electrical behavior and real Wii peripherals are not yet modeled.

## MMIO integration
HollywoodHardware implements the existing MemoryMappedDevice interface and map_into() installs the 0x400-byte register window at 0x0D000000 and its 0x0D800000 mirror. Accesses are 32-bit, naturally aligned, big-endian through the memory bus. The bus-facing default access is Broadway; explicit read_as / write_as methods apply CPU-specific permissions for Starlet-aware integration. Unknown registers and unsupported widths are rejected rather than silently inventing register behavior.

## References
- [WiiBrew: Hollywood registers](https://wiibrew.org/wiki/Hollywood/Registers)
- [WiiBrew: Hollywood IRQs](https://wiibrew.org/wiki/Hollywood/IRQs)
- [WiiBrew: Hollywood GPIOs](https://wiibrew.org/wiki/Hollywood/GPIOs)
- [WiiBrew: Hardware IPC](https://wiibrew.org/wiki/Hardware/IPC)
- [WiiBrew: Main memory](https://wiibrew.org/wiki/Hardware/Main_Memory)

## Remaining Phase 6 work
The Hollywood interrupt/IPC/GPIO foundations now exist, but Phase 6 is not complete. Remaining hardware includes the Hollywood timer/alarm and watchdog, reset and clock/PLL controls, AHB arbitration and protection, SRAM/boot ROM interfaces, NAND/SD/USB/Wi-Fi/Bluetooth/disc controllers, DSP/GPU interfaces, device timing, and integration with the running Broadway and Starlet execution environments. Register permissions and semantics must continue to be checked against hardware documentation and verified tests. Phase 7 remains blocked.
