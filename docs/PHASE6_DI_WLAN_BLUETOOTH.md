# Phase 6 peripheral batch: DI, WLAN SDIO and Bluetooth HCI

## Drive Interface (DI)
The DI component can mount/eject a caller-supplied byte image and perform bounded read-only byte-range access. It does not yet decode the DI command-buffer protocol, perform DMA into emulated RAM, parse Wii optical-disc filesystem structures, or model drive interrupts and timing. The hardware register window remains separate from this image backend.

## WLAN SDIO
The WLAN model exposes two bounded SDIO function address spaces with byte and bulk read/write operations. It provides deterministic function-memory backing for future SDIO command and BCM4318 integration. It does not implement the SDHCI command/response engine, WLAN firmware, 802.11 MAC/PHY, radio behavior, or networking.

## Bluetooth HCI
The HCI transport now processes a deliberately small host-command subset: HCI Reset and Read BD_ADDR, with Command Complete responses and an Unknown HCI Command status for unsupported opcodes. WiiSystem now exposes this command execution through the USB1 permission gate, with an integration regression test. Packet framing and the existing bounded host/device queues remain validated. This is not a BCM2045 firmware implementation, USB enumeration, pairing, inquiry, or Wii Remote emulation.

Hardware references:
- [WiiBrew: Drive Interface](https://wiibrew.org/wiki/Hardware/Drive_Interface)
- [WiiBrew: 802.11 Wireless](https://wiibrew.org/wiki/Hardware/802.11_Wireless)
- [WiiBrew: Bluetooth Card](https://wiibrew.org/wiki/Hardware/Bluetooth_Card)


## IPC interrupt latching correction
Hollywood IPC control writes now latch IRQ 30/31 when an enabled Y/X flag is acknowledged, including a write that clears the flag. The source flags remain pending in the Hollywood interrupt controller until the owning CPU clears the corresponding IRQ flag register with write-one-to-clear. Regression tests cover Broadway and Starlet acknowledgement paths and clearing the latched IRQ.
