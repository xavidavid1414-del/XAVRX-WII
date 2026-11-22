# CPU Foundation, Integer Execution, Control Flow and Exceptions

## Architectural state
- 32 general-purpose registers (GPR), each 32 bits.
- 32 floating-point registers (FPR), each represented as 64 raw bits.
- 32-bit PC, LR, CTR, XER, MSR, SRR0, SRR1, and FPSCR storage.
- 32-bit condition register divided into eight 4-bit fields, indexed most-significant field first.

## Integer execution
The integer executor implements ADDI, ADDIS, ORI, ORIS, XORI, XORIS, ANDI., ADD, SUBF, AND, OR, XOR, and signed register CMP. Unsupported encodings are rejected without changing architectural state.

## Control flow
The control-flow executor supports relative and absolute B, conditional BC with CR-bit and CTR tests, BLR, BCTR, and link forms. Displacements are sign-extended and targets are word-aligned. CTR-based conditional branches decrement CTR only when requested by BO; BCTR rejects encodings that would decrement its target register. BLR captures its old LR target before applying LK.

## Traps and exceptions
TWI and TW evaluate signed and unsigned trap options and enter the program-exception vector (0x700) when a selected condition matches. System calls use vector 0xC00; external interrupts use vector 0x500 and are accepted only when MSR[EE] is set. Exception entry saves PC/MSR in SRR0/SRR1, clears MSR[EE] and MSR[PR], and jumps to the vector. RFI restores PC and MSR from SRR0/SRR1.

This is an initial execution model, not a complete Gekko implementation. Privilege checks, detailed exception syndrome bits, precise exception-specific SRR0 rules, decrementer interrupts, and complete MSR semantics remain future work.

## Testing
Tests cover integer operations and CR updates; signed/absolute/conditional branches; CTR decrement; LR/CTR indirect branches; link behavior; TWI/TW conditions; system-call, program, and external exception entry; interrupt masking; and RFI restoration.


## Broadway execution integration
The emulator now has a Broadway stepping path that fetches 32-bit instructions from the Wii memory bus, routes the implemented integer/control-flow/load-store subset to its executor, and advances the system scheduler by one reference CPU cycle after a successful step. LWZ and STW use the CPU-aware Wii memory bus and preserve big-endian word semantics. Pending enabled PI interrupt causes enter the external-interrupt vector when MSR[EE] permits it.

This is intentionally a first integration slice: it is not a complete Gekko interpreter, does not model precise bus exceptions or instruction-specific cycle costs, and does not execute Starlet ARM code. Unsupported instructions are reported rather than silently treated as successful.
