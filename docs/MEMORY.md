# XAVRX-WII Memory Subsystem

## RAM address range
The memory bus owns a configurable contiguous RAM range in the 32-bit Gekko address space. The range must be non-empty and must not wrap beyond 0xFFFFFFFF. The configured size is exposed for diagnostics and tests.

## Access widths and byte order
The bus supports 8-, 16-, 32-, and 64-bit reads and writes. Multi-byte accesses must be naturally aligned. Bytes are assembled and stored in big-endian order, independent of the host CPU. Unsupported widths return InvalidWidth; unaligned accesses return Misaligned.

## Mapping and errors
Memory-mapped devices implement the MemoryMappedDevice interface and are mapped into non-overlapping address ranges. Device objects are non-owning and must outlive the Memory object. Each callback receives a byte offset and access width. Device failures return DeviceError. Unmapped and boundary-crossing accesses return Unmapped, and do not partially modify RAM or output values.

## Current limitations
This is a reusable bus foundation, not the complete Wii physical/virtual memory map. Cached and uncached aliases, MMU/BAT translation, protection, DMA, atomic reservation operations, cache coherency, and individual Hollywood/Gekko register blocks are not implemented in this phase.
