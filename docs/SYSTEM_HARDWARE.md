# XAVRX-WII System Hardware Foundation

## Clock domains and generic scheduling
The initial clock tree exposes reference domains for Broadway CPU (729 MHz), GPU (243 MHz), bus (162 MHz), DSP (121.5 MHz), and Starlet ARM (243 MHz). These are model reference rates, not a claim that every PLL mode or timing relationship is reproduced. Four generic deterministic timers advance when the host scheduler supplies cycles; they are scaffolding and are not substitutes for the Starlet hardware timer below.

## Wii MEM1 and MEM2
The Wii memory object models 24 MiB of MEM1 and 64 MiB of MEM2 with shared backing across cached, uncached, and physical aliases.

| Bank | Cached CPU view | Uncached CPU view | Physical view | Capacity |
| --- | --- | --- | --- | --- |
| MEM1 | 0x80000000 | 0xC0000000 | 0x00000000 | 24 MiB |
| MEM2 | 0x90000000 | 0xD0000000 | 0x10000000 | 64 MiB |

Access widths, natural alignment, bank boundaries, and big-endian reads/writes are checked. MMU/BAT translation, cache behavior, MEM2 reservation/protection, and the complete memory map are not yet implemented.

## Starlet internal SRAM and BOOT0 mapping
The Starlet memory model provides independent 64 KiB SRAM bank A and 32 KiB SRAM bank B, the 128 KiB window at 0x0D400000, and the 1 MiB high-address mirror range beginning at 0xFFF00000. The seven 128 KiB windows below 0xFFFE0000 mirror the 0x0D400000 window. The 0xFFFE0000 and 0xFFFF0000 windows have their documented special SRAM/BOOT0 roles. HW_SRNPROT (0x60) bit 5 selects the bank arrangement; bit 3 gates Broadway access to SRAM. HW_BOOT0 (0x18C) bit 12 hides BOOT0 and exposes the selected SRAM arrangement. Switching these registers changes the mapping without moving SRAM contents.
The 8 KiB BOOT0 image is not fabricated or embedded. A caller must supply the exact ROM image through the explicit load interface; until then, mapped BOOT0 reads fail instead of returning invented instructions. ROM is read-only. Tests use a synthetic fixture solely to verify mapping, mirroring, repetition, endianness, and access gating; it is not represented as Nintendo firmware.

## Hollywood interrupt controller
The model implements independent 32-bit Broadway and Starlet IRQ flag/mask pairs at offsets 0x30, 0x34, 0x38, and 0x3C, plus the Starlet FIQ mask at 0x40. Sources can be routed to Broadway, Starlet, or both. Flags are write-one-to-clear; masks gate each processor's interrupt independently. The FIQ output is derived from Starlet pending flags AND the FIQ mask. Current event sources include Starlet timer (0), GPIOB (10), GPIO (11), IPC-to-Broadway (30), and IPC-to-Starlet (31). Other IRQ sources still need their corresponding devices and event sources.

## Starlet timer and alarm
The documented Starlet timer register at offset 0x10 is a 32-bit read-only counter; the alarm register at 0x14 is write-only and accepts a counter deadline. Advancing the modeled Starlet timer ticks across the programmed deadline raises Hollywood IRQ 0 for Starlet. The alarm is one-shot and can be acknowledged through the Starlet IRQ flag register (0x38). The host scheduler supplies timer ticks explicitly; a hardware clock-divider and wall-clock conversion are not claimed. Tests cover normal expiry and 32-bit counter wraparound.

## Hollywood IPC
The four documented 32-bit registers are modeled at offsets 0x00–0x0C: PPCMSG, PPCCTRL, ARMMSG, and ARMCTRL. The two message words are shared and freely readable/writable by both CPUs. X1/X2 are Broadway-owned; Y1/Y2 are Starlet-owned. The opposite CPU may read and clear the flags but cannot set them. Broadway alone writes PPCCTRL; Starlet alone writes ARMCTRL. IY1/IY2 enable IRQ 30 on Broadway for Y1/Y2; IX1/IX2 enable IRQ 31 on Starlet for X1/X2. IOS itself, request queues, DMA, and the full Starlet software environment are not modeled.

## Hollywood GPIO
The 24-pin GPIO implementation uses one shared set of output, direction, input, interrupt-level, interrupt-flag, and interrupt-mask state. The Broadway GPIOB view is masked by GPIO_OWNER; Starlet's full view can inspect all pins. Broadway writes only owner-selected pins through GPIOB; Starlet writes only pins not assigned to Broadway through the full bank. GPIO_ENABLE gates pins. Interrupt flags latch when enabled pins reach their configured interrupt level and are W1C-clearable only after the pin returns to its idle (opposite) level. Masked owner-selected flags raise IRQ 10 to Broadway; unowned flags raise IRQ 11 to Starlet. GPIO_STRAPS is read-only in this model and host input is supplied through set_inputs().

## AHB protection, clock and reset registers
HW_AHBPROT at offset 0x64 is modeled as a Starlet-writable protection register. Broadway may read it but cannot grant itself access. Its documented PPCKERN bit (31) enables Broadway's full Hollywood register view; without that bit, Starlet-private registers remain inaccessible to Broadway. The model now decodes the independent PPC and IOP permission bits for SD0/SD1, USB0/USB1, EHCI, SHA, AES, and flash/NAND through an explicit engine-access query. Starlet SRAM additionally enforces its SRNPROT AHPEN, OH1EN, FLAEN, SHAEN, and AESEN gates for Broadway, USB1, flash/NAND, SHA, and AES bus-master accesses. These checks are tested independently. The corresponding storage, USB, and crypto engines are not yet implemented, so the permissions are not yet exercised by full device transactions.
The Starlet-only HW_CLOCKS register at offset 0x190 stores the documented FX and SPEED control bits. The modeled clock-mode query reports 243 MHz when SPEED is clear and 162 MHz when set. HW_RESETS at 0x194 stores the documented writable reset-control bits, masking the reserved top bit. The disk-interface-related reset lines are named separately: bit 10 is RSTB_IODI (DI I/O reset), and bit 3 is RSTB_DIRSTB (DI reset-B). A cleared bit means its reset line is asserted; a set bit means it is released. These are exposed as line states only; the DI controller and reset side effects are not yet implemented. Regression tests independently toggle both lines. The AHB permission regression suite also checks all eight PPC and all eight IOP engine permission bits independently, ensuring each permission grants only its corresponding bus initiator. These are otherwise register-state models only: the full PLL programming sequence, clock-domain reconfiguration, reset sequencing, and reset side effects on individual peripherals are not yet wired into the system. The model does not claim to reproduce power-on register values.

## MMIO integration and system composition
Hollywood registers are 32-bit, naturally aligned, big-endian. Broadway and Starlet access views are installed into the same WiiMemory object at 0x0D000000 and 0x0D800000. The bus selects the correct CPU-specific device view from the initiating CPU, while MEM1/MEM2 remain shared between both processors. Broadway access to Starlet-private registers is rejected unless PPCKERN is set; Starlet receives the full view. The generic MemoryMappedDevice bus is also retained for isolated component tests.
WiiSystem composes WiiMemory, generic system hardware, and HollywoodHardware. Emulator::initialize() creates and maps this hardware composition; reset() resets the RAM and modeled hardware state. This is hardware-system initialization only, not a playable emulation loop or firmware boot.

## References
- [WiiBrew: Hollywood registers](https://wiibrew.org/wiki/Hollywood/Registers)
- [WiiBrew: Hollywood IRQs](https://wiibrew.org/wiki/Hollywood/IRQs)
- [WiiBrew: Hollywood GPIOs](https://wiibrew.org/wiki/Hollywood/GPIOs)
- [WiiBrew: Hardware IPC](https://wiibrew.org/wiki/Hardware/IPC)
- [WiiBrew: Main memory](https://wiibrew.org/wiki/Hardware/Main_Memory)
- [EmuDocs: Starlet timer and alarm](https://emudocs.layle.dev/GameCube%20%26%20Wii/Starlet/)
- [EmuDocs: Starlet SRAM and BOOT0 mapping](https://emudocs.layle.dev/GameCube%20%26%20Wii/Starlet/)

## Remaining Phase 6 work
Phase 6 remains open. Still missing NAND/SD/USB/Wi-Fi/Bluetooth/disc controllers, DSP/GPU devices, full PLL programming and clock-domain reconfiguration, complete reset side effects on each peripheral, device timing, and integration with running Broadway and Starlet execution environments. No watchdog register has been established in the documented Hollywood register set used here; no speculative watchdog MMIO is fabricated. Register semantics and permissions must continue to be checked against hardware documentation and regression tests. Phase 7 remains blocked.
