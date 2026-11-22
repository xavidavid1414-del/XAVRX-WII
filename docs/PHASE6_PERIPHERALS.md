# XAVRX-WII System Hardware Foundation

## Phase 6 peripheral batch: NAND, SD and EHCI

The NAND model now includes a sparse 512 MiB-equivalent flash geometry (4096 blocks, 64 pages per block, 2048 data bytes plus 64 spare bytes per page). Unallocated pages read as erased 0xFF. Page programming enforces NAND's one-way 1-to-0 bit transitions; block erase returns its pages to the erased state. The sparse representation allocates storage only for programmed pages. This models flash storage operations through a component API; the NAND controller's full command sequencer, ECC engine, DMA bus-master path, bad-block behavior and Hollywood IRQ timing are not implemented.

The SD host now accepts a removable card image whose size is a non-zero multiple of 512 bytes. It supports bounded block reads and writes and explicit attach/detach. This provides deterministic backing storage for controller integration, but does not yet implement the complete SDHC command/response state machine, DMA, card initialization protocol, SDIO, interrupts or device-specific timing. The existing MMIO window remains a register-access foundation.

The EHCI model now exposes read-only capability identity, USBCMD run/reset behavior, USBSTS write-one-to-clear status, interrupt enable, frame index, schedule-base alignment checks, configuration flag and a basic root-port connection state. A bounded transfer queue completes one queued transfer per microframe only while the controller is running and a device is connected/enabled. This is a deterministic controller-level transfer scaffold, not a complete EHCI descriptor DMA engine, USB enumeration, periodic/async schedule walker, split transactions or real USB device emulation.

These three components are still not sufficient to boot or play Wii software. Continue validating register semantics against the Wii hardware references and expand DMA, command sequencing, interrupt routing and timing in later Phase 6 batches.

## References
- [WiiBrew: NAND Interface](https://wiibrew.org/wiki/Hardware/NAND_Interface)
- [WiiBrew: NAND flash geometry](https://wiibrew.org/wiki/Hardware/NAND)
- [WiiBrew: SD Host Controller](https://wiibrew.org/wiki/Hardware/SD_Host_Controller)
- [WiiBrew: USB Host Controller / EHCI](https://wiibrew.org/wiki/Hardware/USB_Host_Controller)
