# Phase 6 peripheral batch: DI, WLAN SDIO and Bluetooth HCI

## Drive Interface (DI)
The DI component can mount/eject a caller-supplied byte image and perform bounded read-only byte-range access. It does not yet decode the DI command-buffer protocol, perform DMA into emulated RAM, parse Wii optical-disc filesystem structures, or model drive interrupts and timing. The hardware register window remains separate from this image backend.

## WLAN SDIO
The WLAN model exposes two bounded SDIO function address spaces with byte and bulk read/write operations. It provides deterministic function-memory backing for future SDIO command and BCM4318 integration. It does not implement the SDHCI command/response engine, WLAN firmware, 802.11 MAC/PHY, radio behavior, or networking.

## Bluetooth HCI
The HCI transport now processes a deliberately small host-command subset: HCI Reset and Read BD_ADDR, with Command Complete responses and an Unknown HCI Command status for unsupported opcodes. HCI Reset also clears pending host-to-device and device-to-host transport queues before returning its successful Command Complete event. WiiSystem now exposes this command execution through the USB1 permission gate, with an integration regression test. Packet framing and the existing bounded host/device queues remain validated. This is not a BCM2045 firmware implementation, USB enumeration, pairing, inquiry, or Wii Remote emulation.

Hardware references:
- [WiiBrew: Drive Interface](https://wiibrew.org/wiki/Hardware/Drive_Interface)
- [WiiBrew: 802.11 Wireless](https://wiibrew.org/wiki/Hardware/802.11_Wireless)
- [WiiBrew: Bluetooth Card](https://wiibrew.org/wiki/Hardware/Bluetooth_Card)


## IPC interrupt latching correction
Hollywood IPC control writes now latch IRQ 30/31 when an enabled Y/X flag is acknowledged, including a write that clears the flag. The source flags remain pending in the Hollywood interrupt controller until the owning CPU clears the corresponding IRQ flag register with write-one-to-clear. Regression tests cover Broadway and Starlet acknowledgement paths and clearing the latched IRQ.


## GPIO level-sensitive IRQ reassertion
When Broadway or Starlet acknowledges its Hollywood IRQ flag register, the GPIO interrupt outputs are reevaluated immediately. A still-active, enabled and unmasked GPIO level therefore reasserts its routed IRQ without waiting for another input transition. Regression coverage acknowledges IRQ 10 while its source pin remains active.


## DI command/DMA integration
The Broadway and Starlet DI MMIO views now dispatch the documented 0xA8 read command when DICR is written with TSTART and DMA enabled. The command uses CMDBUF1 as the byte offset, CMDBUF2 and DILENGTH as the transfer size, and DIMAR as the destination. Transfers are synchronous, read-only, and atomically committed to the selected CPU's shared Wii RAM view. TSTART self-clears after dispatch. Unsupported opcodes, missing images, mismatched lengths, and invalid RAM destinations fail without a partial RAM write. This remains a narrow command subset: the full DISR interrupt/status behavior, cover state, asynchronous timing, and other DI opcodes are not implemented.

Reference: [WiiBrew Drive Interface register map](https://wiibrew.org/wiki/Hardware/Drive_Interface) and [WiiBrew /dev/di command notes](https://wiibrew.org/wiki//dev/di).


## DI DMA alignment and direction checks
The supported read-DMA command now rejects a non-read transfer direction, source offsets, RAM destinations, or lengths that violate the 32-byte DMA alignment requirement. Regression tests cover misaligned length, misaligned destination, and unsupported write direction.

## Broadway DSP mailbox readback
The documented inbound DSP mailbox halfwords at offsets 0x00 and 0x02 now support 16-bit CPU readback as well as writes. The pending/set bit in the high halfword remains visible until the DSP-side consumer takes the message; consuming the message clears that bit. Regression coverage checks both halfwords before consumption and verifies the assembled 32-bit message afterward.

Reference: [WiiBrew Hardware/DSP — DSP Interface](https://wiibrew.org/wiki/Hardware/DSP).
