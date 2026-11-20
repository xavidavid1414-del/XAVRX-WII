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
    case 19: { // mcrf: move one condition-register field
        const std::uint32_t bf = (instruction >> 23) & 7u;
        const std::uint32_t bfa = (instruction >> 18) & 7u;
        const std::uint32_t destination_shift = 28u - bf * 4u;
        const std::uint32_t source_shift = 28u - bfa * 4u;
        const std::uint32_t field = (state_.cr >> source_shift) & 0xfu;
        const std::uint32_t mask = 0xfu << destination_shift;
        state_.cr = (state_.cr & ~mask) | (field << destination_shift);
        break;
    }
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
    case 23: { // rlwnm: rotate left word using shift amount from RB, then mask
        const std::uint32_t rs = rt;
        const std::uint32_t rb = (instruction >> 11) & 31u;
        const std::uint32_t shift = state_.gpr[rb] & 31u;
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
        case 19: // mfcr: move condition register to a GPR
            state_.gpr[rt] = state_.cr;
            break;
        case 144: { // mtcrf: move selected GPR fields into the condition register
            const std::uint32_t fxm = (instruction >> 12) & 0xffu;
            const std::uint32_t source = state_.gpr[rs];
            for (std::uint32_t field = 0; field < 8; ++field) {
                if ((fxm & (1u << (7u - field))) == 0) continue;
                const std::uint32_t shift = 28u - field * 4u;
                const std::uint32_t mask = 0xfu << shift;
                state_.cr = (state_.cr & ~mask) | (source & mask);
            }
            break;
        }
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
        case 10: { // addc: add and record carry in XER[CA]
            const std::uint64_t sum = static_cast<std::uint64_t>(a) + b;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            break;
        }
        case 8: // subfc: RB - RA and record no-borrow carry in XER[CA]
            state_.gpr[rt] = b - a;
            state_.xer = (state_.xer & ~(1u << 29)) |
                         (b >= a ? (1u << 29) : 0u);
            break;
        case 266: // add
            state_.gpr[rt] = a + b;
            break;
        case 235: // mullw: multiply low 32 bits
            state_.gpr[rt] = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b));
            break;
        case 491: { // divw: signed word division
            const std::int32_t dividend = static_cast<std::int32_t>(a);
            const std::int32_t divisor = static_cast<std::int32_t>(b);
            if (divisor == 0 ||
                (dividend == (-2147483647 - 1) && divisor == -1)) {
                state_.gpr[rt] = 0;
            } else {
                state_.gpr[rt] = static_cast<std::uint32_t>(dividend / divisor);
            }
            break;
        }
        case 459: { // divwu: unsigned word division
            if (b == 0) {
                state_.gpr[rt] = 0;
            } else {
                state_.gpr[rt] = a / b;
            }
            break;
        }
        case 75: { // mulhw: signed high word of product
            const std::int64_t product =
                static_cast<std::int64_t>(static_cast<std::int32_t>(a)) *
                static_cast<std::int64_t>(static_cast<std::int32_t>(b));
            state_.gpr[rt] = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(product) >> 32);
            break;
        }
        case 11: { // mulhwu: unsigned high word of product
            const std::uint64_t product =
                static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b);
            state_.gpr[rt] = static_cast<std::uint32_t>(product >> 32);
            break;
        }
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
        case 124: // nor
            state_.gpr[ra] = ~(state_.gpr[rs] | b);
            break;
        case 284: // eqv: bitwise equivalence
            state_.gpr[ra] = ~(state_.gpr[rs] ^ b);
            break;
        case 476: // nand
            state_.gpr[ra] = ~(state_.gpr[rs] & b);
            break;
        case 26: { // cntlzw: count leading zero bits
            std::uint32_t value = state_.gpr[rs];
            std::uint32_t count = 0;
            while (count < 32u && (value & 0x80000000u) == 0) {
                ++count;
                value <<= 1;
            }
            state_.gpr[ra] = count;
            break;
        }
        case 954: // extsb: sign-extend byte
            state_.gpr[ra] = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(static_cast<std::int8_t>(state_.gpr[rs])));
            break;
        case 922: // extsh: sign-extend halfword
            state_.gpr[ra] = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(static_cast<std::int16_t>(state_.gpr[rs])));
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
            const bool shifted_out_ones = shift >= 32
                ? value != 0u
                : shift != 0u && (value & ((1u << shift) - 1u)) != 0u;
            state_.xer = (state_.xer & ~(1u << 29)) |
                         (shifted_out_ones ? (1u << 29) : 0u);
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
            const bool shifted_out_ones =
                shift != 0u && (value & ((1u << shift) - 1u)) != 0u;
            state_.xer = (state_.xer & ~(1u << 29)) |
                         (shifted_out_ones ? (1u << 29) : 0u);
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
