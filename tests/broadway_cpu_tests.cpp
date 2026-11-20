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
    memory.write32_be(24, (14u << 26) | (6u << 21) | 7u);

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
    assert(cpu.state().gpr[6] == 7);
    assert(cpu.state().pc == 28);
}
