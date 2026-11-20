#include "xavrx/broadway_cpu.hpp"

#include <cassert>
#include <bit>
#include <cstdint>

int main() {
    xavrx::Memory memory(512);
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
    // Record-form logical instructions update CR0, including XER[SO].
    auto record_logic_state = cpu.state();
    record_logic_state.pc = 24;
    record_logic_state.gpr[3] = 0x80000000u;
    record_logic_state.gpr[4] = 0u;
    record_logic_state.xer |= 1u << 31;
    record_logic_state.cr = 0x01234567u;
    cpu.set_state(record_logic_state);
    memory.write32_be(24, xform(3, 8, 4, 28) | 1u); // and. r8,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[8] == 0u);
    assert((cpu.state().cr >> 28) == 3u); // EQ | SO
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);

    // Immediate multiply and XOR.
    memory.write32_be(84, (7u << 26) | (11u << 21) | (3u << 16) | 0xfffeu);
    memory.write32_be(88, (26u << 26) | (3u << 21) | (12u << 16) | 0x00ffu);
    auto immediate_state = cpu.state();
    immediate_state.gpr[3] = 0x0f0fu;
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
    logical_state.xer &= ~(1u << 31); // Isolate this CR0 test from prior SO checks.
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

    // Rc variants of arithmetic instructions update CR0 and preserve XER[SO].
    auto record_arithmetic_state = cpu.state();
    record_arithmetic_state.pc = 16;
    record_arithmetic_state.gpr[3] = 0xffffffffu;
    record_arithmetic_state.gpr[4] = 1u;
    record_arithmetic_state.xer = 1u << 31; // SO
    record_arithmetic_state.cr = 0x01234567u;
    cpu.set_state(record_arithmetic_state);
    memory.write32_be(16, xform(5, 3, 4, 266) | 1u); // add. r5,r3,r4
    memory.write32_be(20, xform(6, 3, 4, 40) | 1u);  // subf. r6,r3,r4
    assert(cpu.step());
    assert(cpu.state().gpr[5] == 0u);
    assert((cpu.state().cr >> 28) == 3u); // EQ | SO
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);
    assert(cpu.step());
    assert(cpu.state().gpr[6] == 2u);    assert((cpu.state().cr >> 28) == 5u); // GT | SO

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

    // addze adds XER[CA] to RA and records CR0 only when Rc is set.
    auto addze_state = cpu.state();
    addze_state.pc = 16;
    addze_state.gpr[3] = 41u;
    addze_state.xer |= 1u << 29;
    addze_state.xer |= 1u << 31;
    addze_state.cr = 0x01234567u;
    cpu.set_state(addze_state);
    memory.write32_be(16, xform(11, 3, 0, 202)); // addze r11,r3
    memory.write32_be(20, xform(12, 3, 0, 202) | 1u); // addze. r12,r3
    assert(cpu.step());
    assert(cpu.state().gpr[11] == 42u);
    assert(cpu.state().cr == 0x01234567u);
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 41u); // First addze cleared CA because no carry-out.
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);

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


    // Rc variants of multiply/divide instructions update CR0 and retain XER[SO].
    auto record_muldiv_state = cpu.state();
    record_muldiv_state.pc = 0;
    record_muldiv_state.gpr[3] = 0x80000000u;
    record_muldiv_state.gpr[4] = 2u;
    record_muldiv_state.xer |= 1u << 31;
    record_muldiv_state.cr = 0x01234567u;
    cpu.set_state(record_muldiv_state);
    memory.write32_be(0, xform(10, 3, 4, 235) | 1u); // mullw.
    memory.write32_be(4, xform(11, 3, 4, 491) | 1u); // divw.
    memory.write32_be(8, xform(12, 3, 4, 459) | 1u); // divwu.
    memory.write32_be(12, xform(13, 3, 4, 75) | 1u); // mulhw.
    memory.write32_be(16, xform(14, 3, 4, 11) | 1u); // mulhwu.
    assert(cpu.step());
    assert(cpu.state().gpr[10] == 0u);
    assert((cpu.state().cr >> 28) == 3u); // EQ | SO
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);
    assert(cpu.step());
    assert(cpu.state().gpr[11] == 0xc0000000u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 0x40000000u);
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[13] == 0xffffffffu); // signed high word: (-2^31 * 2) >> 32
    assert((cpu.state().cr >> 28) == 9u); // LT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[14] == 1u);
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);

    // Rc variants of shifts and sign-extension instructions update CR0.
    auto record_shift_state = cpu.state();
    record_shift_state.pc = 200;
    record_shift_state.xer |= 1u << 31;
    record_shift_state.gpr[3] = 0x00f00000u;
    record_shift_state.gpr[5] = 0x80u;
    record_shift_state.gpr[6] = 0x8001u;
    record_shift_state.gpr[7] = 1u;
    record_shift_state.gpr[8] = 4u;
    record_shift_state.gpr[9] = 0x80000000u;
    record_shift_state.gpr[10] = 0xf0000000u;
    cpu.set_state(record_shift_state);
    memory.write32_be(200, xform(3, 15, 0, 26) | 1u);  // cntlzw.
    memory.write32_be(204, xform(5, 16, 0, 954) | 1u); // extsb.
    memory.write32_be(208, xform(6, 17, 0, 922) | 1u); // extsh.
    memory.write32_be(212, xform(7, 18, 8, 24) | 1u);  // slw.
    memory.write32_be(216, xform(9, 19, 8, 536) | 1u); // srw.
    memory.write32_be(220, xform(10, 20, 8, 792) | 1u); // sraw.
    memory.write32_be(224, xform(10, 21, 4, 824) | 1u); // srawi.
    assert(cpu.step());
    assert(cpu.state().gpr[15] == 8u);
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[16] == 0xffffff80u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[17] == 0xffff8001u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[18] == 16u);
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[19] == 0x08000000u);
    assert((cpu.state().cr >> 28) == 5u); // GT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[20] == 0xff000000u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO
    assert(cpu.step());
    assert(cpu.state().gpr[21] == 0xff000000u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO

    // mfspr/mtspr transfer the supported XER, LR and CTR registers.
    auto spr_state = cpu.state();
    spr_state.pc = 228;
    spr_state.lr = 0x11223344u;
    spr_state.ctr = 0x55667788u;
    spr_state.xer = 0xaabbccddu;
    spr_state.gpr[25] = 0x01020304u;
    spr_state.gpr[26] = 0x05060708u;
    spr_state.gpr[27] = 0x090a0b0cu;
    cpu.set_state(spr_state);
    const auto spr_xform = [](std::uint32_t reg, std::uint32_t spr,
                              std::uint32_t xo) {
        return (31u << 26) | (reg << 21) | ((spr & 31u) << 16) |
               (((spr >> 5) & 31u) << 11) | (xo << 1);
    };
    memory.write32_be(228, spr_xform(22, 8, 339)); // mfspr r22,LR
    memory.write32_be(232, spr_xform(23, 9, 339)); // mfspr r23,CTR
    memory.write32_be(236, spr_xform(24, 1, 339)); // mfspr r24,XER
    memory.write32_be(240, spr_xform(25, 8, 467)); // mtspr LR,r25
    memory.write32_be(244, spr_xform(26, 9, 467)); // mtspr CTR,r26
    memory.write32_be(248, spr_xform(27, 1, 467)); // mtspr XER,r27
    assert(cpu.step());
    assert(cpu.state().gpr[22] == 0x11223344u);
    assert(cpu.step());
    assert(cpu.state().gpr[23] == 0x55667788u);
    assert(cpu.step());
    assert(cpu.state().gpr[24] == 0xaabbccddu);
    assert(cpu.step());
    assert(cpu.state().lr == 0x01020304u);
    assert(cpu.step());
    assert(cpu.state().ctr == 0x05060708u);
    assert(cpu.step());
    assert(cpu.state().xer == 0x090a0b0cu);

    // An unsupported SPR must be rejected without changing CPU state.
    auto unsupported_spr_state = cpu.state();
    unsupported_spr_state.pc = 252;
    cpu.set_state(unsupported_spr_state);
    memory.write32_be(252, spr_xform(22, 2, 339)); // unsupported SPR
    assert(!cpu.step());
    assert(cpu.state().pc == 252u);
    assert(cpu.state().gpr[22] == unsupported_spr_state.gpr[22]);


    // CR logical instructions read CR bits BA/BB and write only BT.
    auto cr_logic_state = cpu.state();
    cr_logic_state.pc = 256;
    cr_logic_state.cr = (1u << (31u - 6u)); // BA=1, BB=0
    cpu.set_state(cr_logic_state);
    const auto cr_logic = [](std::uint32_t bt, std::uint32_t ba,
                             std::uint32_t bb, std::uint32_t xo) {
        return (19u << 26) | (bt << 21) | (ba << 16) |
               (bb << 11) | (xo << 1);
    };
    const std::uint32_t cr_ops[] = {
        257u, 129u, 449u, 417u, 193u, 289u, 225u, 33u
    };
    const bool cr_expected[] = {
        false, true, true, true, true, false, true, false
    };
    for (std::uint32_t i = 0; i < 8u; ++i) {
        memory.write32_be(256u + i * 4u, cr_logic(5, 6, 7, cr_ops[i]));
        assert(cpu.step());
        const bool actual = ((cpu.state().cr >> (31u - 5u)) & 1u) != 0;
        assert(actual == cr_expected[i]);
    }

    // mcrxr transfers XER[SO,OV,CA] into the selected CR field and clears them.
    auto mcrxr_state = cpu.state();
    mcrxr_state.pc = 288;
    mcrxr_state.xer = 0xe0001234u;
    mcrxr_state.cr = 0x12345678u;
    cpu.set_state(mcrxr_state);
    memory.write32_be(288, (31u << 26) | (3u << 23) | (512u << 1));
    assert(cpu.step());
    assert(((cpu.state().cr >> 16) & 0xfu) == 0xeu);
    assert((cpu.state().cr & 0xfff0ffffu) == (0x12345678u & 0xfff0ffffu));
    assert(cpu.state().xer == 0x00001234u);
    assert(cpu.state().pc == 292u);

    // neg and neg. compute two's-complement values and record CR0 when Rc=1.
    auto negate_state = cpu.state();
    negate_state.pc = 292;
    negate_state.gpr[3] = 7u;
    negate_state.gpr[4] = 0x80000000u;
    negate_state.xer = 1u << 31; // Preserve SO in the recorded CR0 field.
    negate_state.cr = 0x01234567u;
    cpu.set_state(negate_state);
    memory.write32_be(292, xform(5, 3, 0, 104));       // neg r5,r3
    memory.write32_be(296, xform(6, 4, 0, 104) | 1u); // neg. r6,r4
    assert(cpu.step());
    assert(cpu.state().gpr[5] == 0xfffffff9u);
    assert(cpu.state().cr == 0x01234567u);
    assert(cpu.step());
    assert(cpu.state().gpr[6] == 0x80000000u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO
    assert((cpu.state().cr & 0x0fffffffu) == 0x01234567u);
    assert(cpu.state().pc == 300u);

    // addme computes RA - 1 + XER[CA] and updates carry-out.
    auto addme_state = cpu.state();
    addme_state.pc = 24;
    addme_state.gpr[3] = 0u;
    addme_state.xer &= ~(1u << 29);
    cpu.set_state(addme_state);
    memory.write32_be(24, xform(13, 3, 0, 234)); // addme r13,r3
    assert(cpu.step());
    assert(cpu.state().gpr[13] == 0xffffffffu);
    assert((cpu.state().xer & (1u << 29)) == 0u);

    auto addme_carry_state = cpu.state();
    addme_carry_state.pc = 28;
    addme_carry_state.gpr[3] = 0u;
    addme_carry_state.xer |= 1u << 29;
    cpu.set_state(addme_carry_state);
    memory.write32_be(28, xform(14, 3, 0, 234) | 1u); // addme. r14,r3
    assert(cpu.step());
    assert(cpu.state().gpr[14] == 0u);
    assert((cpu.state().xer & (1u << 29)) != 0u);
    assert((cpu.state().cr >> 28) == 3u); // EQ | SO

    // addze records carry-out when RA is 0xffffffff and CA is set.
    auto addze_carry_state = cpu.state();
    addze_carry_state.pc = 48;
    addze_carry_state.gpr[3] = 0xffffffffu;
    addze_carry_state.xer |= 1u << 29;
    cpu.set_state(addze_carry_state);
    memory.write32_be(48, xform(19, 3, 0, 202)); // addze r19,r3
    assert(cpu.step());
    assert(cpu.state().gpr[19] == 0u);
    assert((cpu.state().xer & (1u << 29)) != 0u);

    // subfze computes ~RA + XER[CA] and records carry-out.
    auto subfze_state = cpu.state();
    subfze_state.pc = 32;
    subfze_state.gpr[3] = 0u;
    subfze_state.xer &= ~(1u << 29);
    cpu.set_state(subfze_state);
    memory.write32_be(32, xform(15, 3, 0, 200)); // subfze r15,r3
    assert(cpu.step());
    assert(cpu.state().gpr[15] == 0xffffffffu);
    assert((cpu.state().xer & (1u << 29)) == 0u);

    auto subfze_carry_state = cpu.state();
    subfze_carry_state.pc = 36;
    subfze_carry_state.gpr[3] = 0u;
    subfze_carry_state.xer |= 1u << 29;
    cpu.set_state(subfze_carry_state);
    memory.write32_be(36, xform(16, 3, 0, 200)); // subfze r16,r3
    assert(cpu.step());
    assert(cpu.state().gpr[16] == 0u);
    assert((cpu.state().xer & (1u << 29)) != 0u);

    // subfme computes ~RA + XER[CA] - 1 and records carry-out.
    auto subfme_state = cpu.state();
    subfme_state.pc = 40;
    subfme_state.gpr[3] = 0u;
    subfme_state.xer &= ~(1u << 29);
    cpu.set_state(subfme_state);
    memory.write32_be(40, xform(17, 3, 0, 232)); // subfme r17,r3
    assert(cpu.step());
    assert(cpu.state().gpr[17] == 0xfffffffeu);
    assert((cpu.state().xer & (1u << 29)) != 0u);

    auto subfme_carry_state = cpu.state();
    subfme_carry_state.pc = 44;
    subfme_carry_state.gpr[3] = 0u;
    subfme_carry_state.xer |= 1u << 29;
    cpu.set_state(subfme_carry_state);
    memory.write32_be(44, xform(18, 3, 0, 232) | 1u); // subfme. r18,r3
    assert(cpu.step());
    assert(cpu.state().gpr[18] == 0xffffffffu);
    assert((cpu.state().xer & (1u << 29)) != 0u);
    assert((cpu.state().cr >> 28) == 9u); // LT | SO

    // Big-endian 64-bit memory access used by floating-point loads/stores.
    memory.write64_be(400, 0x3ff8000000000000ull); // 1.5
    assert(memory.read64_be(400) == 0x3ff8000000000000ull);

    // Foundational FPU: lfd/stfd, arithmetic, sign operations and comparison.
    auto fpu_state = cpu.state();
    fpu_state.pc = 324;
    fpu_state.gpr[3] = 400u;
    fpu_state.fpr[1] = 0x4000000000000000ull; // 2.0
    fpu_state.fpr[2] = 0x4008000000000000ull; // 3.0
    cpu.set_state(fpu_state);
    const auto fform = [](std::uint32_t frt, std::uint32_t fra,
                          std::uint32_t frb, std::uint32_t xo) {
        return (63u << 26) | (frt << 21) | (fra << 16) |
               (frb << 11) | (xo << 1);
    };
    memory.write32_be(324, (50u << 26) | (4u << 21) | (3u << 16)); // lfd f4,0(r3)
    memory.write32_be(328, fform(5, 1, 2, 21)); // fadd f5,f1,f2
    memory.write32_be(332, fform(6, 2, 1, 20)); // fsub f6,f2,f1
    memory.write32_be(336, fform(7, 1, 2, 18)); // fdiv f7,f1,f2
    memory.write32_be(340, fform(8, 0, 1, 264)); // fabs f8,f1
    memory.write32_be(344, fform(0, 1, 2, 0)); // fcmpu cr0,f1,f2
    memory.write32_be(348, (54u << 26) | (5u << 21) | (3u << 16)); // stfd f5,0(r3)
    assert(cpu.step());
    assert(cpu.state().fpr[4] == 0x3ff8000000000000ull);
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[5]) == 5.0);
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[6]) == 1.0);
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[7]) == (2.0 / 3.0));
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[8]) == 2.0);
    assert(cpu.step());
    assert((cpu.state().cr >> 28) == 8u); // 2.0 < 3.0
    assert(cpu.step());
    assert(memory.read64_be(400) == std::bit_cast<std::uint64_t>(5.0));

    // Single-precision arithmetic rounds each result to binary32.
    auto single_arithmetic_state = cpu.state();
    single_arithmetic_state.pc = 368;
    single_arithmetic_state.fpr[1] = std::bit_cast<std::uint64_t>(1.0 / 3.0);
    single_arithmetic_state.fpr[2] = std::bit_cast<std::uint64_t>(2.0);
    cpu.set_state(single_arithmetic_state);
    memory.write32_be(368, (59u << 26) | (10u << 21) | (1u << 16) | (2u << 11) | (21u << 1)); // fadds f10,f1,f2
    memory.write32_be(372, (59u << 26) | (11u << 21) | (1u << 16) | (2u << 11) | (18u << 1)); // fdivs f11,f1,f2
    memory.write32_be(376, (59u << 26) | (12u << 21) | (1u << 16) | (2u << 6) | (25u << 1)); // fmuls f12,f1,f2
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[10]) ==
           static_cast<double>(static_cast<float>(1.0 / 3.0) + 2.0f));
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[11]) ==
           static_cast<double>(static_cast<float>(1.0 / 3.0) / 2.0f));
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[12]) ==
           static_cast<double>(static_cast<float>(1.0 / 3.0) * 2.0f));

    // lfs/stfs convert between binary32 memory and binary64 FPR storage.
    auto single_state = cpu.state();
    single_state.pc = 352;
    single_state.gpr[3] = 360u;
    cpu.set_state(single_state);
    memory.write32_be(360, std::bit_cast<std::uint32_t>(1.25f));
    memory.write32_be(352, (48u << 26) | (9u << 21) | (3u << 16));
    memory.write32_be(356, (52u << 26) | (9u << 21) | (3u << 16) | 4u);
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[9]) == 1.25);
    assert(cpu.step());
    assert(memory.read32_be(364) == std::bit_cast<std::uint32_t>(1.25f));

    // Update-form loads/stores write the effective address back to RA.
    auto update_memory_state = cpu.state();
    update_memory_state.pc = 400;
    update_memory_state.gpr[3] = 448u;
    update_memory_state.gpr[4] = 464u;
    update_memory_state.gpr[6] = 0xaabbccddu;
    update_memory_state.fpr[1] = std::bit_cast<std::uint64_t>(2.5);
    update_memory_state.fpr[2] = std::bit_cast<std::uint64_t>(1.25);
    cpu.set_state(update_memory_state);
    memory.write32_be(456, 0x12345678u);
    memory.write32_be(460, std::bit_cast<std::uint32_t>(3.5f));
    memory.write64_be(464, std::bit_cast<std::uint64_t>(1.5));
    memory.write32_be(400, (33u << 26) | (5u << 21) | (3u << 16) | 8u); // lwzu r5,8(r3)
    memory.write32_be(404, (37u << 26) | (6u << 21) | (4u << 16) | 8u); // stwu r6,8(r4)
    memory.write32_be(408, (49u << 26) | (7u << 21) | (3u << 16) | 4u); // lfsu f7,4(r3)
    memory.write32_be(412, (53u << 26) | (1u << 21) | (4u << 16) | 4u); // stfsu f1,4(r4)
    memory.write32_be(416, (51u << 26) | (8u << 21) | (3u << 16) | 4u); // lfdu f8,4(r3)
    memory.write32_be(420, (55u << 26) | (2u << 21) | (4u << 16) | 4u); // stfdu f2,4(r4)
    assert(cpu.step());
    assert(cpu.state().gpr[5] == 0x12345678u);
    assert(cpu.state().gpr[3] == 456u);
    assert(cpu.step());
    assert(memory.read32_be(472) == 0xaabbccddu);
    assert(cpu.state().gpr[4] == 472u);
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[7]) == 3.5);
    assert(cpu.state().gpr[3] == 460u);
    assert(cpu.step());
    assert(memory.read32_be(476) == std::bit_cast<std::uint32_t>(2.5f));
    assert(cpu.state().gpr[4] == 476u);
    assert(cpu.step());
    assert(std::bit_cast<double>(cpu.state().fpr[8]) == 1.5);
    assert(cpu.state().gpr[3] == 464u);
    assert(cpu.step());
    assert(memory.read64_be(480) == std::bit_cast<std::uint64_t>(1.25));
    assert(cpu.state().gpr[4] == 480u);

    // Update-form instructions with RA=r0 are invalid and leave state unchanged.
    auto invalid_update_state = cpu.state();
    invalid_update_state.pc = 440;
    invalid_update_state.gpr[5] = 0xfeedbeefu;
    cpu.set_state(invalid_update_state);
    memory.write32_be(440, (33u << 26) | (5u << 21) | 4u); // lwzu r5,4(r0)
    assert(!cpu.step());
    assert(cpu.state().pc == 440u);
    assert(cpu.state().gpr[5] == 0xfeedbeefu);

    // Byte and halfword loads/stores, sign extension, update forms and alignment.
    auto narrow_memory_state = cpu.state();
    narrow_memory_state.pc = 448;
    narrow_memory_state.gpr[3] = 480u;
    narrow_memory_state.gpr[4] = 0x89abcdefu;
    cpu.set_state(narrow_memory_state);
    memory.write8(480, 0x80u);
    memory.write16_be(482, 0x8001u);
    memory.write8(484, 0x80u);
    memory.write32_be(448, (34u << 26) | (10u << 21) | (3u << 16)); // lbz r10,0(r3)
    memory.write32_be(452, (42u << 26) | (11u << 21) | (3u << 16) | 2u); // lha r11,2(r3)
    memory.write32_be(456, (40u << 26) | (12u << 21) | (3u << 16) | 2u); // lhz r12,2(r3)
    memory.write32_be(460, (35u << 26) | (13u << 21) | (3u << 16) | 4u); // lbzu r13,4(r3)
    memory.write32_be(464, (38u << 26) | (4u << 21) | (3u << 16) | 8u); // stb r4,8(r3)
    memory.write32_be(468, (44u << 26) | (4u << 21) | (3u << 16) | 10u); // sth r4,10(r3)
    memory.write32_be(472, (39u << 26) | (4u << 21) | (3u << 16) | 12u); // stbu r4,12(r3)
    assert(cpu.step());
    assert(cpu.state().gpr[10] == 0x80u);
    assert(cpu.step());
    assert(cpu.state().gpr[11] == 0xffff8001u);
    assert(cpu.step());
    assert(cpu.state().gpr[12] == 0x8001u);
    assert(cpu.step());
    assert(cpu.state().gpr[13] == 0x80u);
    assert(cpu.state().gpr[3] == 484u);
    assert(cpu.step());
    assert(memory.read8(488) == 0xefu);
    assert(cpu.step());
    assert(memory.read16_be(490) == 0xcdefu);
    assert(cpu.step());
    assert(memory.read8(492) == 0xefu);
    assert(cpu.state().gpr[3] == 492u);

    // Update-form stores reject RA=0 and RA=RS without changing CPU state.
    auto invalid_narrow_update = cpu.state();
    invalid_narrow_update.pc = 476;
    invalid_narrow_update.gpr[3] = 496u;
    cpu.set_state(invalid_narrow_update);
    memory.write32_be(476, (39u << 26) | (3u << 21) | (3u << 16) | 4u);
    assert(!cpu.step());
    assert(cpu.state().pc == 476u);
    assert(cpu.state().gpr[3] == 496u);

    // Structured CPU exceptions: fetch, illegal instruction and data faults.
    auto exception_state = cpu.state();
    exception_state.pc = 401u;
    cpu.set_state(exception_state);
    assert(!cpu.step());
    assert(cpu.state().exception == xavrx::BroadwayCpu::Exception::Alignment);
    assert(cpu.state().pc == 401u);

    exception_state = cpu.state();
    exception_state.pc = 512u;
    cpu.set_state(exception_state);
    assert(!cpu.step());
    assert(cpu.state().exception == xavrx::BroadwayCpu::Exception::InstructionStorage);

    exception_state = cpu.state();
    exception_state.pc = 384u;
    cpu.set_state(exception_state);
    memory.write32_be(384, 0u); // Primary opcode zero is unsupported.
    assert(!cpu.step());
    assert(cpu.state().exception == xavrx::BroadwayCpu::Exception::IllegalInstruction);
    assert(cpu.state().pc == 384u);

    exception_state = cpu.state();
    exception_state.pc = 388u;
    exception_state.gpr[3] = 512u;
    cpu.set_state(exception_state);
    memory.write32_be(388, (32u << 26) | (10u << 21) | (3u << 16)); // lwz r10,0(r3)
    assert(!cpu.step());
    assert(cpu.state().exception == xavrx::BroadwayCpu::Exception::DataStorage);
    assert(cpu.state().pc == 388u);

    exception_state = cpu.state();
    exception_state.pc = 392u;
    exception_state.gpr[3] = 321u;
    cpu.set_state(exception_state);
    memory.write32_be(392, (32u << 26) | (10u << 21) | (3u << 16)); // unaligned lwz
    assert(!cpu.step());
    assert(cpu.state().exception == xavrx::BroadwayCpu::Exception::Alignment);
    assert(cpu.state().pc == 392u);

    return 0;
}
