# CPU Foundation and Integer Execution
## Architectural state
- 32 general-purpose registers (GPR), each 32 bits.
- 32 floating-point registers (FPR), each represented as 64 raw bits.
- 32-bit PC, LR, CTR, XER, MSR, SRR0, SRR1, and FPSCR storage.
- 32-bit condition register divided into eight 4-bit fields, indexed most-significant field first.

## Decode and execution
The decoder identifies a documented initial subset of Gekko/PowerPC encodings. The integer executor currently implements ADDI, ADDIS, ORI, ORIS, XORI, XORIS, ANDI., ADD, SUBF, AND, OR, XOR, and signed register CMP. Supported instructions update the destination register and advance PC by four; record-form operations update CR0, and CMP writes the selected condition-register field.

The executor deliberately rejects unsupported overflow-enabled arithmetic encodings and 64-bit compare forms rather than silently applying incorrect semantics. Loads/stores, branches, exception transitions, floating-point execution, full XER carry/overflow behavior, and complete instruction validation are not implemented in this phase.

## Testing
Tests cover register state, decoder formats, immediate sign extension, zero-base ADDI, shifted immediates, logical results, arithmetic, CR recording, signed comparison, PC advancement, and unsupported/invalid instruction state preservation.
