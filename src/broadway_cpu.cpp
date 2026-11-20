#include "xavrx/broadway_cpu.hpp"

#include <cstdint>

namespace xavrx {
namespace {
std::int32_t sign_extend_16(std::uint32_t value) noexcept {
    return static_cast<std::int16_t>(value & 0xffffu);
}

std::uint32_t rotate_left_32(std::uint32_t value, std::uint32_t amount) noexcept {
    amount &= 31u;
    return amount == 0 ? value : (value << amount) | (value >> (32u - amount));
}

void update_cr0(BroadwayCpu::State& state, std::uint32_t value) noexcept {
    const std::uint32_t field = (value & 0x80000000u) != 0 ? 8u
        : value == 0 ? 2u : 4u;
    state.cr = (state.cr & 0x0fffffffu) | (field << 28);
}

std::uint32_t ppc_mask(std::uint32_t begin, std::uint32_t end) noexcept {
    std::uint32_t mask = 0;
    for (std::uint32_t bit = 0; bit < 32; ++bit) {
        const bool selected = begin <= end
            ? (bit >= begin && bit <= end)
            : (bit >= begin || bit <= end);
        if (selected) mask |= (1u << (31u - bit));
    }
    return mask;
}
}

BroadwayCpu::BroadwayCpu(Memory& memory, std::uint32_t entry) noexcept
    : memory_(memory) {
    state_.pc = entry;
}

const BroadwayCpu::State& BroadwayCpu::state() const noexcept {
    return state_;
}

void BroadwayCpu::set_state(const State& state) noexcept {
    state_ = state;
}

bool BroadwayCpu::step() {
    const std::uint32_t instruction = memory_.read32_be(state_.pc);
    const std::uint32_t opcode = instruction >> 26;
    const std::uint32_t rt = (instruction >> 21) & 31u;
    const std::uint32_t ra = (instruction >> 16) & 31u;
    const std::uint32_t immediate = instruction & 0xffffu;
    const std::uint32_t next_pc = state_.pc + 4u;
    const std::uint32_t base = ra == 0 ? 0u : state_.gpr[ra];

    switch (opcode) {
    case 20: { // rlwimi: rotate left, then insert selected bits into RA
        const std::uint32_t rs = rt;
        const std::uint32_t shift = (instruction >> 11) & 31u;
        const std::uint32_t mb = (instruction >> 6) & 31u;
        const std::uint32_t me = (instruction >> 1) & 31u;
        const std::uint32_t mask = ppc_mask(mb, me);
        const std::uint32_t rotated = rotate_left_32(state_.gpr[rs], shift);
        state_.gpr[ra] = (state_.gpr[ra] & ~mask) | (rotated & mask);
        break;
    }
    case 21: { // rlwinm: rotate left word immediate then AND with mask
        const std::uint32_t rs = rt;
        const std::uint32_t shift = (instruction >> 11) & 31u;
        const std::uint32_t mb = (instruction >> 6) & 31u;
        const std::uint32_t me = (instruction >> 1) & 31u;
        state_.gpr[ra] = rotate_left_32(state_.gpr[rs], shift) & ppc_mask(mb, me);
        break;
    }
    case 10: // cmpli: compare logical immediate (32-bit)
    case 11: { // cmpi: compare signed immediate (32-bit)
        if ((instruction & (1u << 21)) != 0) return false;
        const std::uint32_t field = (instruction >> 23) & 7u;
        const std::uint32_t lhs = state_.gpr[ra];
        bool less = false;
        bool greater = false;
        if (opcode == 11) {
            const std::int32_t signed_lhs = static_cast<std::int32_t>(lhs);
            const std::int32_t signed_rhs = sign_extend_16(immediate);
            less = signed_lhs < signed_rhs;
            greater = signed_lhs > signed_rhs;
        } else {
            less = lhs < immediate;
            greater = lhs > immediate;
        }
        const std::uint32_t result = less ? 8u : greater ? 4u : 2u;
        const std::uint32_t shift = 28u - (field * 4u);
        const std::uint32_t mask = 0xfu << shift;
        state_.cr = (state_.cr & ~mask) | (result << shift);
        break;
    }
    case 7: // mulli
        state_.gpr[rt] = base * static_cast<std::uint32_t>(sign_extend_16(immediate));
        break;
    case 14: // addi
        state_.gpr[rt] = base + static_cast<std::uint32_t>(sign_extend_16(immediate));
        break;
    case 15: // addis
        state_.gpr[rt] = base +
            (static_cast<std::uint32_t>(sign_extend_16(immediate)) << 16);
        break;
    case 24: // ori (RS is encoded in the RT field)
        state_.gpr[ra] = state_.gpr[rt] | immediate;
        break;
    case 26: // xori (RS is encoded in RT)
        state_.gpr[ra] = state_.gpr[rt] ^ immediate;
        break;
    case 27: // xoris
        state_.gpr[ra] = state_.gpr[rt] ^ (immediate << 16);
        break;
    case 28: { // andi.: AND immediate and record CR0
        const std::uint32_t result = state_.gpr[rt] & immediate;
        state_.gpr[ra] = result;
        update_cr0(state_, result);
        break;
    }
    case 29: { // andis.: AND shifted immediate and record CR0
        const std::uint32_t result = state_.gpr[rt] & (immediate << 16);
        state_.gpr[ra] = result;
        update_cr0(state_, result);
        break;
    }
    case 25: // oris
        state_.gpr[ra] = state_.gpr[rt] | (immediate << 16);
        break;
    case 32: // lwz
        state_.gpr[rt] = memory_.read32_be(base + static_cast<std::uint32_t>(
            sign_extend_16(immediate)));
        break;
    case 36: // stw
        memory_.write32_be(base + static_cast<std::uint32_t>(
            sign_extend_16(immediate)), state_.gpr[rt]);
        break;
    case 16: { // bc: conditional branch
        const std::uint32_t bo = (instruction >> 21) & 31u;
        const std::uint32_t bi = (instruction >> 16) & 31u;
        std::int32_t displacement = static_cast<std::int32_t>(
            instruction & 0x0000fffcu);
        if ((displacement & 0x00008000) != 0) {
            displacement |= static_cast<std::int32_t>(0xffff0000u);
        }
        const bool absolute = (instruction & 2u) != 0;
        const bool link = (instruction & 1u) != 0;

        bool ctr_ok = true;
        if ((bo & 4u) == 0) {
            --state_.ctr;
            ctr_ok = ((state_.ctr != 0) != ((bo & 2u) != 0));
        }
        const bool condition = ((state_.cr >> (31u - bi)) & 1u) != 0;
        const bool condition_ok = (bo & 16u) != 0 ||
                                  (condition == ((bo & 8u) != 0));
        if (link) state_.lr = next_pc;
        if (ctr_ok && condition_ok) {
            state_.pc = absolute ? static_cast<std::uint32_t>(displacement)
                                 : state_.pc + static_cast<std::uint32_t>(displacement);
        } else {
            state_.pc = next_pc;
        }
        return true;
    }
    case 18: { // b
        std::int32_t displacement = static_cast<std::int32_t>(
            instruction & 0x03fffffcu);
        if ((displacement & 0x02000000) != 0) {
            displacement |= static_cast<std::int32_t>(0xfc000000u);
        }
        const bool absolute = (instruction & 2u) != 0;
        const bool link = (instruction & 1u) != 0;
        if (link) state_.lr = next_pc;
        state_.pc = absolute ? static_cast<std::uint32_t>(displacement)
                             : state_.pc + static_cast<std::uint32_t>(displacement);
        return true;
    }
    case 31: { // X-form integer instructions
        const std::uint32_t rs = rt;
        const std::uint32_t rb = (instruction >> 11) & 31u;
        const std::uint32_t xo = (instruction >> 1) & 0x3ffu;
        const std::uint32_t a = state_.gpr[ra];
        const std::uint32_t b = state_.gpr[rb];
        switch (xo) {
        case 528: { // bctr: conditional branch to CTR
            const std::uint32_t bo = rt;
            const std::uint32_t bi = ra;
            if ((bo & 4u) == 0) return false; // bctr does not decrement CTR
            const std::uint32_t target = state_.ctr & ~3u;
            const bool condition = ((state_.cr >> (31u - bi)) & 1u) != 0;
            const bool condition_ok = (bo & 16u) != 0 ||
                                      (condition == ((bo & 8u) != 0));
            if ((instruction & 1u) != 0) state_.lr = next_pc;
            state_.pc = condition_ok ? target : next_pc;
            return true;
        }
        case 16: { // bclr: conditional branch to LR
            const std::uint32_t bo = rt;
            const std::uint32_t bi = ra;
            const std::uint32_t target = state_.lr & ~3u;
            bool ctr_ok = true;
            if ((bo & 4u) == 0) {
                --state_.ctr;
                ctr_ok = ((state_.ctr != 0) != ((bo & 2u) != 0));
            }
            const bool condition = ((state_.cr >> (31u - bi)) & 1u) != 0;
            const bool condition_ok = (bo & 16u) != 0 ||
                                      (condition == ((bo & 8u) != 0));
            if ((instruction & 1u) != 0) state_.lr = next_pc;
            state_.pc = ctr_ok && condition_ok ? target : next_pc;
            return true;
        }
        case 0: // cmp: compare signed 32-bit registers
        case 32: { // cmpl: compare unsigned 32-bit registers
            if ((instruction & (1u << 21)) != 0) return false;
            const std::uint32_t field = (instruction >> 23) & 7u;
            bool less = false;
            bool greater = false;
            if (xo == 0) {
                const std::int32_t signed_a = static_cast<std::int32_t>(a);
                const std::int32_t signed_b = static_cast<std::int32_t>(b);
                less = signed_a < signed_b;
                greater = signed_a > signed_b;
            } else {
                less = a < b;
                greater = a > b;
            }
            const std::uint32_t result = less ? 8u : greater ? 4u : 2u;
            const std::uint32_t shift = 28u - (field * 4u);
            const std::uint32_t mask = 0xfu << shift;
            state_.cr = (state_.cr & ~mask) | (result << shift);
            break;
        }
        case 266: // add
            state_.gpr[rt] = a + b;
            break;
        case 40: // subf: RT = RB - RA
            state_.gpr[rt] = b - a;
            break;
        case 28: // and
            state_.gpr[ra] = state_.gpr[rs] & b;
            break;
        case 444: // or
            state_.gpr[ra] = state_.gpr[rs] | b;
            break;
        case 316: // xor
            state_.gpr[ra] = state_.gpr[rs] ^ b;
            break;
        case 24: // slw: shift left word
            state_.gpr[ra] = (b & 0x20u) != 0 ? 0u : state_.gpr[rs] << (b & 0x3fu);
            break;
        case 536: // srw: shift right word
            state_.gpr[ra] = (b & 0x20u) != 0 ? 0u : state_.gpr[rs] >> (b & 0x3fu);
            break;
        case 792: { // sraw: arithmetic shift right word
            const std::uint32_t value = state_.gpr[rs];
            const std::uint32_t shift = b & 0x3fu;
            if (shift == 0) {
                state_.gpr[ra] = value;
            } else if (shift >= 32) {
                state_.gpr[ra] = (value & 0x80000000u) != 0 ? 0xffffffffu : 0u;
            } else {
                const std::uint32_t fill = (value & 0x80000000u) != 0
                    ? (~0u << (32u - shift)) : 0u;
                state_.gpr[ra] = (value >> shift) | fill;
            }
            break;
        }
        case 824: { // srawi: arithmetic shift right word immediate
            const std::uint32_t value = state_.gpr[rs];
            const std::uint32_t shift = rb;
            if (shift == 0) {
                state_.gpr[ra] = value;
            } else {
                const std::uint32_t fill = (value & 0x80000000u) != 0
                    ? (~0u << (32u - shift)) : 0u;
                state_.gpr[ra] = (value >> shift) | fill;
            }
            break;
        }
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }

    state_.pc = next_pc;
    return true;
}

} // namespace xavrx
