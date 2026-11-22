# Phase 6 peripheral batch: DI, WLAN SDIO and Bluetooth HCI

## Drive Interface (DI)
The DI component can mount/eject a caller-supplied byte image and perform bounded read-only byte-range access. Its Broadway/Starlet MMIO views implement a narrow synchronous 0xA8 read-DMA path into shared emulated RAM with alignment, direction, length and destination validation. The full DI command-buffer protocol, DISR/DICVR status and interrupt behavior, asynchronous timing, other opcodes, and optical-disc filesystem parsing remain unimplemented.

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
The documented inbound DSP mailbox halfwords at offsets 0x00 and 0x02 support 16-bit CPU readback and writes. A high-halfword write alone does not publish a message: the low-halfword write commits it only when the high-halfword Set bit (bit 15) is set. The Set bit remains asserted while the message is pending and is cleared when the DSP-side consumer receives the assembled 32-bit payload exactly once. Regression coverage verifies the pre-commit state, commit ordering, Set-bit persistence until receive, clearing on receive, duplicate-consumption rejection, and ignored data when Set is clear.

Reference: [WiiBrew Hardware/DSP — DSP Interface](https://wiibrew.org/wiki/Hardware/DSP).


## DI status and cover register model
The DI MMIO device now models DISR (DEINT/TCINT/BRKINT flags and masks), DICVR (read-only cover state, cover interrupt mask and W1C flag), and DICR (DMA/direction bits with TSTART readback clear). Successful synchronous 0xA8 DMA latches TCINT; rejected/failed started commands latch DEINT. The new dedicated DI test target checks mask gating, W1C behavior, cover-state readback, reset behavior and the end-to-end MMIO DMA completion path. The processor/Starlet IRQ line routing and asynchronous command timing are still not implemented.

Reference: [Dolphin DVDInterface register definitions and interrupt update logic](https://github.com/dolphin-emu/dolphin/blob/a76221bc3109da6147d7e22b941c64712103dd6e/Source/Core/Core/HW/DVD/DVDInterface.h) and [DVDInterface.cpp](https://github.com/dolphin-emu/dolphin/blob/a76221bc3109da6147d7e22b941c64712103dd6e/Source/Core/Core/HW/DVD/DVDInterface.cpp).

The DI interrupt source is also connected to Hollywood IRQ 18 for both Broadway and Starlet. The IRQ flags remain latched in the Hollywood interrupt controller until each CPU acknowledges its own flag register; the DI source flags are independently cleared through DISR/DICVR W1C writes. WiiBrew's Hollywood IRQ table identifies source 18 as the Drive Interface and documents the independent per-CPU flag/mask registers.

DI cover transitions are event-driven: mounting a disc changes CVR from open to closed and latches CVRINT; ejecting a mounted disc changes CVR to open and latches CVRINT. Repeating an insertion/ejection operation without a cover-state transition does not create another event.
