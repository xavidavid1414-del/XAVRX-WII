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

    // Exercise X-form register instructions using known register values.
    auto state = cpu.state();
    state.pc = 64;
    state.gpr[3] = 0x0f0fu;
    state.gpr[4] = 0x00ffu;
    cpu.set_state(state);
    const auto xform = [](std::uint32_t rt, std::uint32_t ra,
                          std::uint32_t rb, std::uint32_t xo) {
        return (31u << 26) | (rt << 21) | (ra << 16) |
               (rb << 11) | (xo << 1);
    };
    memory.write32_be(64, xform(6, 3, 4, 266)); // add r6,r3,r4
    memory.write32_be(68, xform(7, 3, 4, 40));  // subf r7,r3,r4
    memory.write32_be(72, xform(3, 8, 4, 28));  // and r8,r3,r4
    memory.write32_be(76, xform(3, 9, 4, 444)); // or r9,r3,r4
    memory.write32_be(80, xform(3, 10, 4, 316)); // xor r10,r3,r4

    assert(cpu.step());
    assert(cpu.state().gpr[6] == 0x100eu);
    assert(cpu.step());
    assert(cpu.state().gpr[7] == 0xfffff1f0u);
    assert(cpu.step());
    assert(cpu.state().gpr[8] == 0x000fu);
    assert(cpu.step());
    assert(cpu.state().gpr[9] == 0x0fffu);
    assert(cpu.step());
    assert(cpu.state().gpr[10] == 0x0ff0u);
    // Immediate multiply and XOR.
    memory.write32_be(84, (7u << 26) | (11u << 21) | (3u << 16) | 0xfffeu);
    memory.write32_be(88, (26u << 26) | (3u << 21) | (12u << 16) | 0x00ffu);
    assert(cpu.step());
    assert(cpu.state().gpr[11] == 0xffffe1e2u);
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 0x0ff0u);

    // Register shifts: slw r13,r3,r4 and srw r14,r3,r4.
    auto shift_state = cpu.state();
    shift_state.pc = 92;
    shift_state.gpr[3] = 0x0000000fu;
    shift_state.gpr[4] = 2u;
    cpu.set_state(shift_state);
    memory.write32_be(92, xform(3, 13, 4, 24));  // slw r13,r3,r4
    memory.write32_be(96, xform(3, 14, 4, 536)); // srw r14,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[13] == 0x0000003cu);
    assert(cpu.step());
    assert(cpu.state().gpr[14] == 0x00000003u);

    // A shift amount of 32 or more produces zero for logical shifts.
    auto large_shift_state = cpu.state();
    large_shift_state.pc = 100;
    large_shift_state.gpr[4] = 32u;
    cpu.set_state(large_shift_state);
    memory.write32_be(100, xform(3, 13, 4, 24));
    assert(cpu.step());
    assert(cpu.state().gpr[13] == 0u);
}
