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
    memory.write32_be(192, xform(3, 10, 4, 124)); // nor r10,r3,r4
    memory.write32_be(196, xform(3, 11, 4, 284)); // eqv r11,r3,r4
    memory.write32_be(200, xform(3, 12, 4, 476)); // nand r12,r3,r4
    auto logic_extra_state = cpu.state();
    logic_extra_state.pc = 192;
    cpu.set_state(logic_extra_state);
    assert(cpu.step());
    assert(cpu.state().gpr[10] == 0xfffff000u);
    assert(cpu.step());
    assert(cpu.state().gpr[11] == 0xfffff00fu);
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 0xfffffff0u);
    // Immediate multiply and XOR.
    memory.write32_be(84, (7u << 26) | (11u << 21) | (3u << 16) | 0xfffeu);
    memory.write32_be(88, (26u << 26) | (3u << 21) | (12u << 16) | 0x00ffu);
    auto immediate_state = cpu.state();
    immediate_state.pc = 84;
    cpu.set_state(immediate_state);
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

    // Arithmetic shifts preserve the sign bit for negative values.
    auto arithmetic_state = cpu.state();
    arithmetic_state.pc = 104;
    arithmetic_state.gpr[3] = 0xfffffff0u; // -16
    arithmetic_state.gpr[4] = 2u;
    cpu.set_state(arithmetic_state);
    memory.write32_be(104, xform(3, 15, 4, 792)); // sraw r15,r3,r4
    memory.write32_be(108, xform(3, 16, 3, 824)); // srawi r16,r3,3
    assert(cpu.step());
    assert(cpu.state().gpr[15] == 0xfffffffcu);
    assert(cpu.step());
    assert(cpu.state().gpr[16] == 0xfffffffeu);

    // rlwinm rotates left then applies PowerPC's MSB-numbered bit mask.
    auto rotate_state = cpu.state();
    rotate_state.pc = 112;
    rotate_state.gpr[3] = 0x12345678u;
    cpu.set_state(rotate_state);
    const auto rlwinm = [](std::uint32_t rs, std::uint32_t ra,
                           std::uint32_t sh, std::uint32_t mb,
                           std::uint32_t me) {
        return (21u << 26) | (rs << 21) | (ra << 16) |
               (sh << 11) | (mb << 6) | (me << 1);
    };
    memory.write32_be(112, rlwinm(3, 17, 8, 8, 23));
    memory.write32_be(116, rlwinm(3, 18, 8, 28, 3));
    assert(cpu.step());
    assert(cpu.state().gpr[17] == 0x00567800u);
    assert(cpu.step());
    assert(cpu.state().gpr[18] == 0x30000002u);

    // rlwimi preserves destination bits outside the selected mask.
    auto insert_state = cpu.state();
    insert_state.gpr[19] = 0xaaaaaaaau;
    insert_state.gpr[20] = 0xaaaaaaaau;
    cpu.set_state(insert_state);
    const auto rlwimi = [](std::uint32_t rs, std::uint32_t ra,
                           std::uint32_t sh, std::uint32_t mb,
                           std::uint32_t me) {
        return (20u << 26) | (rs << 21) | (ra << 16) |
               (sh << 11) | (mb << 6) | (me << 1);
    };
    memory.write32_be(120, rlwimi(3, 19, 8, 8, 23));
    memory.write32_be(124, rlwimi(3, 20, 8, 28, 3));
    assert(cpu.step());
    assert(cpu.state().gpr[19] == 0xaa5678aau);
    assert(cpu.step());
    assert(cpu.state().gpr[20] == 0x3aaaaaa2u);

    // xoris XORs the immediate in the upper 16 bits.
    memory.write32_be(128, (27u << 26) | (3u << 21) | (21u << 16) | 0x00ffu);
    assert(cpu.step());
    assert(cpu.state().gpr[21] == 0x12cb5678u);

    // andi. and andis. update CR0 from the logical result.
    auto logical_state = cpu.state();
    logical_state.gpr[4] = 0xf2345678u;
    logical_state.cr = 0x01234567u;
    cpu.set_state(logical_state);
    memory.write32_be(132, (28u << 26) | (3u << 21) | (22u << 16) | 0x00ffu);
    memory.write32_be(136, (29u << 26) | (4u << 21) | (23u << 16) | 0x8000u);
    assert(cpu.step());
    assert(cpu.state().gpr[22] == 0x78u);
    assert((cpu.state().cr >> 28) == 4u); // GT
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);
    assert(cpu.step());
    assert(cpu.state().gpr[23] == 0x80000000u);
    assert((cpu.state().cr >> 28) == 8u); // LT

    // cmpi and cmpli write the selected CR field without changing GPRs.
    auto compare_state = cpu.state();
    compare_state.pc = 140;
    compare_state.gpr[3] = 0xffffffffu;
    compare_state.cr = 0x01234567u;
    cpu.set_state(compare_state);
    memory.write32_be(140, (11u << 26) | (2u << 23) | (3u << 16)); // cmpi cr2,r3,0
    memory.write32_be(144, (10u << 26) | (1u << 23) | (3u << 16) | 0xffffu); // cmpli cr1,r3,65535
    assert(cpu.step());
    assert(((cpu.state().cr >> 20) & 0xfu) == 8u); // CR2: LT
    assert(cpu.state().gpr[3] == 0xffffffffu);
    assert(cpu.step());
    assert(((cpu.state().cr >> 24) & 0xfu) == 4u); // CR1: GT

    // cmp and cmpl compare GPRs and update the selected CR field.
    auto register_compare_state = cpu.state();
    register_compare_state.pc = 148;
    register_compare_state.gpr[5] = 0xffffffffu;
    register_compare_state.gpr[6] = 1u;
    register_compare_state.cr = 0x01234567u;
    cpu.set_state(register_compare_state);
    const auto compare_xform = [](std::uint32_t bf, std::uint32_t ra,
                                  std::uint32_t rb, std::uint32_t xo) {
        return (31u << 26) | (bf << 23) | (ra << 16) |
               (rb << 11) | (xo << 1);
    };
    memory.write32_be(148, compare_xform(3, 5, 6, 0));  // cmp cr3,r5,r6
    memory.write32_be(152, compare_xform(4, 5, 6, 32)); // cmpl cr4,r5,r6
    assert(cpu.step());
    assert(((cpu.state().cr >> 16) & 0xfu) == 8u); // signed -1 < 1
    assert(cpu.step());
    assert(((cpu.state().cr >> 12) & 0xfu) == 4u); // unsigned max > 1
    assert(cpu.state().gpr[5] == 0xffffffffu);

    // bc branches when the selected CR bit matches BO's expected value.
    auto branch_state = cpu.state();
    branch_state.pc = 156;
    branch_state.cr = 2u << 28; // CR0 EQ bit set
    branch_state.ctr = 9u;
    cpu.set_state(branch_state);
    const auto bc = [](std::uint32_t bo, std::uint32_t bi,
                       std::uint32_t displacement, bool link = false) {
        return (16u << 26) | (bo << 21) | (bi << 16) |
               (displacement & 0xfffcu) | (link ? 1u : 0u);
    };
    memory.write32_be(156, bc(12, 2, 8)); // ignore CTR; branch if CR0 EQ
    memory.write32_be(160, bc(12, 2, 8, true));
    assert(cpu.step());
    assert(cpu.state().pc == 164u);
    assert(cpu.state().ctr == 9u);
    // The same condition is false when CR0 EQ is clear.
    auto not_taken_state = cpu.state();
    not_taken_state.pc = 160;
    not_taken_state.cr = 0;
    cpu.set_state(not_taken_state);
    assert(cpu.step());
    assert(cpu.state().pc == 164u);

    // bclr uses the aligned LR target and the same CR condition logic.
    auto link_branch_state = cpu.state();
    link_branch_state.pc = 172;
    link_branch_state.lr = 200;
    link_branch_state.cr = 2u << 28; // CR0 EQ
    link_branch_state.ctr = 5;
    cpu.set_state(link_branch_state);
    memory.write32_be(172, xform(12, 2, 0, 16)); // bclr: branch if CR0 EQ
    assert(cpu.step());
    assert(cpu.state().pc == 200u);
    assert(cpu.state().ctr == 5u);
    auto link_branch_false = cpu.state();
    link_branch_false.pc = 176;
    link_branch_false.cr = 0;
    cpu.set_state(link_branch_false);
    memory.write32_be(176, xform(12, 2, 0, 16));
    assert(cpu.step());
    assert(cpu.state().pc == 180u);

    // bctr branches to the aligned CTR target when its CR condition matches.
    auto count_branch_state = cpu.state();
    count_branch_state.pc = 184;
    count_branch_state.ctr = 212;
    count_branch_state.cr = 2u << 28; // CR0 EQ
    cpu.set_state(count_branch_state);
    memory.write32_be(184, xform(12, 2, 0, 528)); // bctr if CR0 EQ
    assert(cpu.step());
    assert(cpu.state().pc == 212u);
    assert(cpu.state().ctr == 212u); // bctr does not decrement CTR
    auto count_branch_false = cpu.state();
    count_branch_false.pc = 188;
    count_branch_false.cr = 0;
    cpu.set_state(count_branch_false);
    memory.write32_be(188, xform(12, 2, 0, 528));
    assert(cpu.step());
    assert(cpu.state().pc == 192u);
    // cntlzw counts leading zeroes; extsb/extsh preserve signed values.
    auto extend_state = cpu.state();
    extend_state.pc = 204;
    extend_state.gpr[3] = 0x00100000u;
    extend_state.gpr[4] = 0x00000080u;
    extend_state.gpr[5] = 0x00008001u;
    cpu.set_state(extend_state);
    memory.write32_be(204, xform(3, 6, 0, 26));   // cntlzw r6,r3
    memory.write32_be(208, xform(4, 7, 0, 954));  // extsb r7,r4
    memory.write32_be(212, xform(5, 8, 0, 922));  // extsh r8,r5
    assert(cpu.step());
    assert(cpu.state().gpr[6] == 11u);
    assert(cpu.step());
    assert(cpu.state().gpr[7] == 0xffffff80u);
    assert(cpu.step());
    assert(cpu.state().gpr[8] == 0xffff8001u);

    // mullw stores the low 32 bits of the register product.
    auto multiply_state = cpu.state();
    multiply_state.pc = 216;
    multiply_state.gpr[3] = 0x10000u;
    multiply_state.gpr[4] = 0x10001u;
    cpu.set_state(multiply_state);
    memory.write32_be(216, xform(9, 3, 4, 235)); // mullw r9,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[9] == 0x00010000u);

    // mulhw and mulhwu return the high 32 bits, signed and unsigned.
    auto high_multiply_state = cpu.state();
    high_multiply_state.pc = 220;
    high_multiply_state.gpr[3] = 0x80000000u;
    high_multiply_state.gpr[4] = 4u;
    cpu.set_state(high_multiply_state);
    memory.write32_be(220, xform(10, 3, 4, 75)); // mulhw r10,r3,r4
    memory.write32_be(224, xform(11, 3, 4, 11)); // mulhwu r11,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[10] == 0xfffffffeu);
    assert(cpu.step());
    assert(cpu.state().gpr[11] == 2u);


    // rlwnm rotates by the amount held in RB, then applies the MSB-numbered mask.
    auto variable_rotate_state = cpu.state();
    variable_rotate_state.pc = 228;
    variable_rotate_state.gpr[3] = 0x12345678u;
    variable_rotate_state.gpr[4] = 8u;
    cpu.set_state(variable_rotate_state);
    memory.write32_be(228, (23u << 26) | (3u << 21) | (12u << 16) |
                      (4u << 11) | (8u << 6) | (23u << 1)); // rlwnm r12,r3,r4,8,23
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 0x00567800u);


    // divw: signed division, including divide-by-zero handling.
    auto divide_state = cpu.state();
    divide_state.pc = 232;
    divide_state.gpr[3] = static_cast<std::uint32_t>(-21);
    divide_state.gpr[4] = 4u;
    cpu.set_state(divide_state);
    memory.write32_be(232, xform(12, 3, 4, 491)); // divw r12,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[12] == static_cast<std::uint32_t>(-5));

    auto divide_zero_state = cpu.state();
    divide_zero_state.pc = 236;
    divide_zero_state.gpr[4] = 0u;
    cpu.set_state(divide_zero_state);
    memory.write32_be(236, xform(12, 3, 4, 491));
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 0u);

    // divwu performs unsigned division.
    auto unsigned_divide_state = cpu.state();
    unsigned_divide_state.pc = 240;
    unsigned_divide_state.gpr[3] = 100u;
    unsigned_divide_state.gpr[4] = 9u;
    cpu.set_state(unsigned_divide_state);
    memory.write32_be(240, xform(13, 3, 4, 459)); // divwu r13,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[13] == 11u);

    // addc/subfc update the XER carry bit (bit 29).
    auto carry_state = cpu.state();
    carry_state.pc = 0;
    carry_state.gpr[3] = 0xffffffffu;
    carry_state.gpr[4] = 1u;
    carry_state.xer = 0x12345678u;
    cpu.set_state(carry_state);
    memory.write32_be(0, xform(5, 3, 4, 10)); // addc r5,r3,r4
    memory.write32_be(4, xform(6, 3, 4, 8));  // subfc r6,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[5] == 0u);
    assert((cpu.state().xer & (1u << 29)) != 0u);
    assert(cpu.step());
    assert(cpu.state().gpr[6] == 2u);
    assert((cpu.state().xer & (1u << 29)) == 0u);

    // adde and subfe consume XER[CA] and update the carry-out bit.
    auto extended_arithmetic_state = cpu.state();
    extended_arithmetic_state.pc = 8;
    extended_arithmetic_state.gpr[3] = 0xffffffffu;
    extended_arithmetic_state.gpr[4] = 0u;
    extended_arithmetic_state.gpr[7] = 5u;
    extended_arithmetic_state.gpr[8] = 10u;
    extended_arithmetic_state.xer |= 1u << 29;
    cpu.set_state(extended_arithmetic_state);
    memory.write32_be(8, xform(9, 3, 4, 138));  // adde r9,r3,r4
    memory.write32_be(12, xform(10, 7, 8, 136)); // subfe r10,r7,r8
    assert(cpu.step());
    assert(cpu.state().gpr[9] == 0u);
    assert((cpu.state().xer & (1u << 29)) != 0u);
    assert(cpu.step());
    assert(cpu.state().gpr[10] == 5u);
    assert((cpu.state().xer & (1u << 29)) != 0u);

    // mcrf copies a selected CR field; mfcr reads the full CR into a GPR.
    auto cr_transfer_state = cpu.state();
    cr_transfer_state.pc = 244;
    cr_transfer_state.cr = 0x12345678u;
    cr_transfer_state.gpr[3] = 0xabcdef01u;
    cpu.set_state(cr_transfer_state);
    memory.write32_be(244, (19u << 26) | (2u << 23) | (1u << 18)); // mcrf cr2,cr1
    memory.write32_be(248, (31u << 26) | (5u << 21) | (19u << 1)); // mfcr r5
    memory.write32_be(252, (31u << 26) | (3u << 21) | (0x80u << 12) | (144u << 1)); // mtcrf cr0,r3
    assert(cpu.step());
    assert(((cpu.state().cr >> 20) & 0xfu) == 2u);
    assert(cpu.step());
    assert(cpu.state().gpr[5] == cpu.state().cr);
    assert(cpu.step());
    assert((cpu.state().cr >> 28) == 0xau);
    assert((cpu.state().cr & 0x0fffffffu) == 0x02245678u);

    // sraw/srawi update XER[CA] from one-bits shifted out of the low end.
    auto arithmetic_carry_state = cpu.state();
    arithmetic_carry_state.pc = 0;
    arithmetic_carry_state.gpr[3] = 0xfffffff1u;
    arithmetic_carry_state.gpr[4] = 2u;
    arithmetic_carry_state.xer = 0x12345678u;
    cpu.set_state(arithmetic_carry_state);
    arithmetic_carry_state.gpr[7] = 0xfffffff0u;
    cpu.set_state(arithmetic_carry_state);
    memory.write32_be(0, xform(3, 15, 4, 792)); // sraw r15,r3,r4
    memory.write32_be(4, xform(7, 16, 3, 824)); // srawi r16,r7,3
    assert(cpu.step());
    assert(cpu.state().gpr[15] == 0xfffffffcu);
    assert((cpu.state().xer & (1u << 29)) != 0u);
    assert((cpu.state().xer & ~(1u << 29)) ==
           (arithmetic_carry_state.xer & ~(1u << 29)));
    assert(cpu.step());
    assert(cpu.state().gpr[16] == 0xfffffffeu);
    assert((cpu.state().xer & (1u << 29)) == 0u);

    // CR record fields and compare fields include XER[SO].
    auto summary_overflow_state = cpu.state();
    summary_overflow_state.pc = 16;
    summary_overflow_state.xer |= 1u << 31;
    summary_overflow_state.gpr[3] = 1u;
    summary_overflow_state.gpr[4] = 1u;
    cpu.set_state(summary_overflow_state);
    memory.write32_be(16, (28u << 26) | (3u << 21) | (22u << 16) | 1u); // andi. r22,r3,1
    assert(cpu.step());
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    memory.write32_be(20, (11u << 26) | (2u << 23) | (3u << 16) | 1u); // cmpi cr2,r3,1
    assert(cpu.step());
    assert(((cpu.state().cr >> 20) & 0xfu) == 3u); // EQ | SO

    // A shift of 32 or more discards the entire nonzero source word.
    auto wide_arithmetic_carry_state = cpu.state();
    wide_arithmetic_carry_state.pc = 8;
    wide_arithmetic_carry_state.gpr[3] = 0x80000000u;
    wide_arithmetic_carry_state.gpr[4] = 32u;
    cpu.set_state(wide_arithmetic_carry_state);
    memory.write32_be(8, xform(3, 17, 4, 792)); // sraw r17,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[17] == 0xffffffffu);
    assert((cpu.state().xer & (1u << 29)) != 0u);

}
