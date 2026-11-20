#include "xavrx/broadway_cpu.hpp"

#include <cassert>
#include <cstdint>

int main() {
    xavrx::Memory memory(256);
    // addi r3, r0, 42
    memory.write32_be(0, (14u << 26) | (3u << 21) | 42u);
    // addi r4, r3, -2
    memory.write32_be(4, (14u << 26) | (4u << 21) | (3u << 16) | 0xfffeu);
    // stw r4, 32(r0)
    memory.write32_be(8, (36u << 26) | (4u << 21) | 32u);
    // lwz r5, 32(r0)
    memory.write32_be(12, (32u << 26) | (5u << 21) | 32u);
    // b +8
    memory.write32_be(16, (18u << 26) | 8u);
    // This instruction must be skipped.
    memory.write32_be(20, (14u << 26) | (6u << 21) | 99u);
    // add r7, r3, r4
    memory.write32_be(24, (31u << 26) | (7u << 21) | (3u << 16) |
                           (4u << 11) | (266u << 1));
    // subf r8, r3, r4 => r4 - r3
    memory.write32_be(28, (31u << 26) | (8u << 21) | (3u << 16) |
                           (4u << 11) | (40u << 1));
    // or r9, r3, r4
    memory.write32_be(32, (31u << 26) | (9u << 21) | (3u << 16) |
                           (4u << 11) | (444u << 1));
    // xor r10, r3, r4
    memory.write32_be(36, (31u << 26) | (10u << 21) | (3u << 16) |
                           (4u << 11) | (316u << 1));

    xavrx::BroadwayCpu cpu(memory);
    assert(cpu.step());
    assert(cpu.state().gpr[3] == 42);
    assert(cpu.step());
    assert(cpu.state().gpr[4] == 40);
    assert(cpu.step());
    assert(memory.read32_be(32) == 40);
    assert(cpu.step());
    assert(cpu.state().gpr[5] == 40);
    assert(cpu.step());
    assert(cpu.state().pc == 24);
    assert(cpu.step());
    assert(cpu.state().gpr[7] == 82);
    assert(cpu.step());
    assert(cpu.state().gpr[8] == 0xfffffffeu);
    assert(cpu.step());
    assert(cpu.state().gpr[9] == 42);
    assert(cpu.step());
    assert(cpu.state().gpr[10] == (42u ^ 40u));
    assert(cpu.state().pc == 40);
}
