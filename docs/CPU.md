# CPU Foundation (Gekko)
Phase 2 adds a host-independent Gekko architectural-state model and a decode-only instruction identification layer.

## Architectural state
- 32 general-purpose registers (GPR), each 32 bits.
- 32 floating-point registers (FPR), each represented as 64 raw bits.
- 32-bit PC, LR, CTR, XER, MSR, SRR0, SRR1, and FPSCR storage.
- 32-bit condition register divided into eight 4-bit fields, indexed most-significant field first.
- Invalid register/field indices return zero or ignore writes; this is defensive API behavior, not hardware behavior.

The state object does not yet implement reset vectors, privilege rules, register side effects, or exception transitions.

## Instruction decoder
The decoder recognizes a documented initial subset of primary opcodes and selected extended opcodes in primary-opcode groups 19, 31, and 63. It reports instruction identity and encoding form only. It does not extract operands, validate every reserved bit, or execute instructions. Unsupported encodings return Unknown.

Integer execution is phase 3; control flow and exception behavior are phase 4.

## Tests
The CPU test target covers register widths and bounds, condition-register field ordering and updates, reset behavior, and representative D/I/B/X/XL/XO/A-form decoding.
