# XAVRX-WII System Hardware Foundation

## Phase 6 peripheral batch: NAND, SD and EHCI

The NAND model now includes a sparse 512 MiB-equivalent flash geometry (4096 blocks, 64 pages per block, 2048 data bytes plus 64 spare bytes per page). Unallocated pages read as erased 0xFF. Page programming enforces NAND's one-way 1-to-0 bit transitions; block erase returns its pages to the erased state. The sparse representation allocates storage only for programmed pages. The documented `NAND_UNK` register at offset `0x18` is exposed as an opaque read/write register; no undocumented behavior is assigned to it. This models flash storage operations through a component API; the NAND controller's full command sequencer, ECC engine, DMA bus-master path, bad-block behavior and Hollywood IRQ timing are not implemented.

The SD host now accepts a removable card image whose size is a non-zero multiple of 512 bytes. It supports bounded block reads and writes and explicit attach/detach. This provides deterministic backing storage for controller integration, but does not yet implement the complete SDHC command/response state machine, DMA, card initialization protocol, SDIO, interrupts or device-specific timing. The existing MMIO window remains a register-access foundation.

The Drive Interface can copy a bounded range from a mounted read-only disc image into shared MEM1 through `WiiSystem::read_disc_to_memory`. The transfer validates CPU identity and the complete source-image range before writing, then commits the complete range atomically to a single MEM1 or MEM2 RAM bank. Invalid, unmapped, MMIO, overflowing or cross-bank destinations are rejected without partial writes. Broadway and Starlet observe the same destination bytes. This is a synchronous DMA-style transfer helper, not the hardware DI command sequencer, interrupt state machine or asynchronous DMA engine.

The EHCI model now exposes read-only capability identity, USBCMD run/reset behavior, USBSTS write-one-to-clear status, interrupt enable, frame index, schedule-base alignment checks, configuration flag and a root-port connection state with separate connect-detected and high-speed-enable transitions. PORTSC connect/enable change bits are write-one-to-clear; a connected port is not considered enabled until the modeled high-speed reset completion. Bounded pending and completion queues provide backpressure: a transfer completes at most once per microframe, only while the controller is running and a device is connected/enabled, and pending work remains queued whenever the completion queue reaches capacity. The WiiSystem advances EHCI microframes from the scheduled 162 MHz bus clock at 125 μs intervals, carrying fractional ticks across calls; the controller fast-forwards empty frames without per-frame loops. This is still a deterministic controller-level transfer scaffold with queue-capacity backpressure, not a complete EHCI descriptor DMA engine, USB enumeration, periodic/async schedule walker, split transactions or real USB device emulation.

These three components are still not sufficient to boot or play Wii software. Continue validating register semantics against the Wii hardware references and expand DMA, command sequencing, interrupt routing and timing in later Phase 6 batches.

## References
- [WiiBrew: NAND Interface](https://wiibrew.org/wiki/Hardware/NAND_Interface)
- [WiiBrew: NAND flash geometry](https://wiibrew.org/wiki/Hardware/NAND)
- [WiiBrew: SD Host Controller](https://wiibrew.org/wiki/Hardware/SD_Host_Controller)
- [WiiBrew: USB Host Controller / EHCI](https://wiibrew.org/wiki/Hardware/USB_Host_Controller)


The initial DI command path is limited to the documented read-DMA direction. It now validates 32-byte alignment for the source offset, destination address and transfer length, and rejects write-to-disc direction rather than treating it as a read. The test fixture uses an aligned 32-byte transfer.