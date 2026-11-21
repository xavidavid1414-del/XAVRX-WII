#include "xavrx/broadway_cpu.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

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
    const std::uint32_t relation = (value & 0x80000000u) != 0 ? 8u
        : value == 0 ? 2u : 4u;
    const std::uint32_t summary_overflow = (state.xer >> 31) & 1u;
    const std::uint32_t field = relation | summary_overflow;
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
    state_.exception = Exception::None;
    if ((state_.pc & 3u) != 0u) {
        state_.exception = Exception::Alignment;
        return false;
    }
    if (!memory_.contains(state_.pc, 4)) {
        state_.exception = Exception::InstructionStorage;
        return false;
    }
    try {
        if (execute_one()) {
            ++state_.time_base;
            return true;
        }
        state_.exception = Exception::IllegalInstruction;
        return false;
    } catch (const std::invalid_argument&) {
        state_.exception = Exception::Alignment;
        return false;
    } catch (const std::out_of_range&) {
        state_.exception = Exception::DataStorage;
        return false;
    }
}

bool BroadwayCpu::execute_one() {
    const std::uint32_t instruction = memory_.read32_be(state_.pc);
    const std::uint32_t opcode = instruction >> 26;
    const std::uint32_t rt = (instruction >> 21) & 31u;
    const std::uint32_t ra = (instruction >> 16) & 31u;
    const std::uint32_t immediate = instruction & 0xffffu;
    const std::uint32_t next_pc = state_.pc + 4u;
    const std::uint32_t base = ra == 0 ? 0u : state_.gpr[ra];
    // The 750CL reservation granule is modeled as one 32-byte cache line.
    // Any CPU store to that line invalidates the active reservation.
    const auto invalidate_reservation = [&](std::uint32_t address,
                                            std::uint32_t size) {
        if (!state_.reservation_valid || size == 0u) return;
        const std::uint64_t first = address;
        const std::uint64_t last = first + size;
        const std::uint64_t reserved_line =
            static_cast<std::uint64_t>(state_.reservation_address & ~31u);
        if (first < reserved_line + 32u && last > reserved_line) {
            state_.reservation_valid = false;
        }
    };

    switch (opcode) {
    case 48: // lfs
    case 49: // lfsu
    case 50: // lfd
    case 51: { // lfdu
        const std::uint32_t frt = rt;
        const bool update = opcode == 49u || opcode == 51u;
        if (update && ra == 0u) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        const bool is_double = opcode == 50u || opcode == 51u;
        const std::uint32_t alignment = is_double ? 8u : 4u;
        if ((address & (alignment - 1u)) != 0u) {
            throw std::invalid_argument("unaligned floating-point load");
        }
        if (is_double) {
            state_.fpr[frt] = memory_.read64_be(address);
        } else {
            const float value = std::bit_cast<float>(memory_.read32_be(address));
            state_.fpr[frt] = std::bit_cast<std::uint64_t>(static_cast<double>(value));
        }
        if (update) state_.gpr[ra] = address;
        break;
    }
    case 52: // stfs
    case 53: // stfsu
    case 54: // stfd
    case 55: { // stfdu
        const std::uint32_t frs = rt;
        const bool update = opcode == 53u || opcode == 55u;
        if (update && ra == 0u) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        const bool is_double = opcode == 54u || opcode == 55u;
        const std::uint32_t alignment = is_double ? 8u : 4u;
        if ((address & (alignment - 1u)) != 0u) {
            throw std::invalid_argument("unaligned floating-point store");
        }
        const double value = std::bit_cast<double>(state_.fpr[frs]);
        invalidate_reservation(address, is_double ? 8u : 4u);
        if (is_double) {
            memory_.write64_be(address, state_.fpr[frs]);
        } else {
            memory_.write32_be(address, std::bit_cast<std::uint32_t>(
                static_cast<float>(value)));
        }
        if (update) state_.gpr[ra] = address;
        break;
    }
    case 19: { // CR logical operations and mcrf
        const std::uint32_t xo = (instruction >> 1) & 0x3ffu;
        if (xo == 150u) { // isync: instruction synchronization barrier
            break;
        }
        if (xo == 0) { // mcrf: move one condition-register field
            const std::uint32_t bf = (instruction >> 23) & 7u;
            const std::uint32_t bfa = (instruction >> 18) & 7u;
            const std::uint32_t destination_shift = 28u - bf * 4u;
            const std::uint32_t source_shift = 28u - bfa * 4u;
            const std::uint32_t field = (state_.cr >> source_shift) & 0xfu;
            const std::uint32_t mask = 0xfu << destination_shift;
            state_.cr = (state_.cr & ~mask) | (field << destination_shift);
            break;
        }

        const std::uint32_t bt = (instruction >> 21) & 31u;
        const std::uint32_t ba = (instruction >> 16) & 31u;
        const std::uint32_t bb = (instruction >> 11) & 31u;
        const bool a_bit = ((state_.cr >> (31u - ba)) & 1u) != 0;
        const bool b_bit = ((state_.cr >> (31u - bb)) & 1u) != 0;
        bool result = false;
        switch (xo) {
        case 33:  result = !(a_bit || b_bit); break; // crnor
        case 129: result = a_bit && !b_bit; break;   // crandc
        case 193: result = a_bit != b_bit; break;    // crxor
        case 225: result = !(a_bit && b_bit); break; // crnand
        case 257: result = a_bit && b_bit; break;    // crand
        case 289: result = a_bit == b_bit; break;    // creqv
        case 417: result = a_bit || !b_bit; break;   // crorc
        case 449: result = a_bit || b_bit; break;    // cror
        default: return false;
        }
        const std::uint32_t mask = 1u << (31u - bt);
        state_.cr = result ? (state_.cr | mask) : (state_.cr & ~mask);
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
        const std::uint32_t result = (less ? 8u : greater ? 4u : 2u) |
            ((state_.xer >> 31) & 1u);
        const std::uint32_t shift = 28u - (field * 4u);
        const std::uint32_t mask = 0xfu << shift;
        state_.cr = (state_.cr & ~mask) | (result << shift);
        break;
    }
    case 8: { // subfic: subtract RA from sign-extended immediate
        const std::uint32_t rhs = static_cast<std::uint32_t>(sign_extend_16(immediate));
        state_.gpr[rt] = rhs - state_.gpr[ra];
        state_.xer = (state_.xer & ~(1u << 29)) |
                     (rhs >= state_.gpr[ra] ? (1u << 29) : 0u);
        break;
    }
    case 12: // addic: add immediate and record carry
    case 13: { // addic.: add immediate, record carry and CR0
        const std::uint32_t rhs = static_cast<std::uint32_t>(sign_extend_16(immediate));
        const std::uint64_t sum = static_cast<std::uint64_t>(state_.gpr[ra]) + rhs;
        state_.gpr[rt] = static_cast<std::uint32_t>(sum);
        state_.xer = (state_.xer & ~(1u << 29)) |
                     ((sum >> 32) != 0 ? (1u << 29) : 0u);
        if (opcode == 13u) update_cr0(state_, state_.gpr[rt]);
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
    case 33: { // lwzu: load word and update RA
        const bool update = opcode == 33u;
        if (update && ra == 0u) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((address & 3u) != 0u) throw std::invalid_argument("unaligned lwz");
        const std::uint32_t value = memory_.read32_be(address);
        state_.gpr[rt] = value;
        if (update) state_.gpr[ra] = address;
        break;
    }
    case 34: // lbz
    case 35: // lbzu
    case 40: // lhz
    case 41: // lhzu
    case 42: // lha
    case 43: { // lhau
        const bool update = opcode == 35u || opcode == 41u || opcode == 43u;
        if (update && ra == 0u) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((opcode >= 40u) && (address & 1u) != 0u) {
            throw std::invalid_argument("unaligned halfword load");
        }
        std::uint32_t value = 0;
        if (opcode == 34u || opcode == 35u) {
            value = memory_.read8(address);
        } else {
            const std::uint16_t half = memory_.read16_be(address);
            value = (opcode == 42u || opcode == 43u)
                ? static_cast<std::uint32_t>(static_cast<std::int32_t>(
                    static_cast<std::int16_t>(half)))
                : static_cast<std::uint32_t>(half);
        }
        state_.gpr[rt] = value;
        if (update) state_.gpr[ra] = address;
        break;
    }
    case 38: // stb
    case 39: // stbu
    case 44: // sth
    case 45: { // sthu
        const bool update = opcode == 39u || opcode == 45u;
        if (update && (ra == 0u || ra == rt)) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        const bool halfword = opcode == 44u || opcode == 45u;
        if (halfword && (address & 1u) != 0u) {
            throw std::invalid_argument("unaligned halfword store");
        }
        invalidate_reservation(address, halfword ? 2u : 1u);
        if (halfword) memory_.write16_be(address,
            static_cast<std::uint16_t>(state_.gpr[rt]));
        else memory_.write8(address, static_cast<std::uint8_t>(state_.gpr[rt]));
        if (update) state_.gpr[ra] = address;
        break;
    }
    case 46: // lmw: load multiple words
    case 47: { // stmw: store multiple words
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((address & 3u) != 0u) {
            throw std::invalid_argument("unaligned multiple-word access");
        }
        // Validate the complete range before changing registers or memory.
        const std::size_t word_count = 32u - rt;
        const std::size_t byte_count = word_count * 4u;
        if (!memory_.contains(address, byte_count)) {
            throw std::out_of_range("multiple-word access outside memory");
        }
        if (opcode == 46u) {
            for (std::uint32_t reg = rt; reg < 32u; ++reg) {
                state_.gpr[reg] = memory_.read32_be(
                    address + (reg - rt) * 4u);
            }
        } else {
            invalidate_reservation(address, static_cast<std::uint32_t>(byte_count));
            for (std::uint32_t reg = rt; reg < 32u; ++reg) {
                memory_.write32_be(address + (reg - rt) * 4u,
                                   state_.gpr[reg]);
            }
        }
        break;
    }
    case 36: // stw
    case 37: { // stwu: store word and update RA
        const bool update = opcode == 37u;
        if (update && (ra == 0u || ra == rt)) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((address & 3u) != 0u) throw std::invalid_argument("unaligned stw");
        invalidate_reservation(address, 4u);
        memory_.write32_be(address, state_.gpr[rt]);
        if (update) state_.gpr[ra] = address;
        break;
    }
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
    case 59: // Single-precision floating-point arithmetic
    case 63: { // Double-precision floating-point and conversion instructions
        const std::uint32_t frt = rt;
        const std::uint32_t fra = ra;
        const std::uint32_t frb = (instruction >> 11) & 31u;
        const std::uint32_t frc = (instruction >> 6) & 31u;
        const std::uint32_t xo = (instruction >> 1) & 0x3ffu;
        const std::uint32_t a_xo = (instruction >> 1) & 31u;
        const double a = std::bit_cast<double>(state_.fpr[fra]);
        const double b = std::bit_cast<double>(state_.fpr[frb]);
        auto write_fpr = [&](double value) {
            state_.fpr[frt] = std::bit_cast<std::uint64_t>(value);
        };
        // fsel is an A-form instruction: its low five XO bits must be
        // decoded separately because FRB/FRC share the upper XO bits.
        if (opcode == 63u && a_xo == 23u) {
            const std::uint32_t selected = a >= -0.0 ? frc : frb;
            state_.fpr[frt] = state_.fpr[selected];
            break;
        }
        // A-form arithmetic encodes a five-bit XO; FRC occupies the
        // adjacent field and must not be included in that decode.
        if (a_xo == 18u || a_xo == 20u || a_xo == 21u ||
            a_xo == 25u || a_xo == 28u || a_xo == 29u ||
            a_xo == 30u || a_xo == 31u) {
            const double c_value = std::bit_cast<double>(state_.fpr[frc]);
            if (opcode == 59u) {
                const float lhs = static_cast<float>(a);
                const float rhs = static_cast<float>(b);
                const float third = static_cast<float>(c_value);
                float result = 0.0f;
                switch (a_xo) {
                case 18: result = lhs / rhs; break;       // fdivs
                case 20: result = lhs - rhs; break;       // fsubs
                case 21: result = lhs + rhs; break;       // fadds
                case 25: result = lhs * third; break;     // fmuls
                case 28: result = std::fma(lhs, third, -rhs); break; // fmsubs
                case 29: result = std::fma(lhs, third, rhs); break;  // fmadds
                case 30: result = -std::fma(lhs, third, -rhs); break; // fnmsubs
                case 31: result = -std::fma(lhs, third, rhs); break;  // fnmadds
                default: return false;
                }
                write_fpr(static_cast<double>(result));
            } else {
                double result = 0.0;
                switch (a_xo) {
                case 18: result = a / b; break;             // fdiv
                case 20: result = a - b; break;             // fsub
                case 21: result = a + b; break;             // fadd
                case 25: result = a * c_value; break;       // fmul
                case 28: result = std::fma(a, c_value, -b); break; // fmsub
                case 29: result = std::fma(a, c_value, b); break;  // fmadd
                case 30: result = -std::fma(a, c_value, -b); break; // fnmsub
                case 31: result = -std::fma(a, c_value, b); break;  // fnmadd
                default: return false;
                }
                write_fpr(result);
            }
            break;
        }
        if (opcode == 59u) return false;
        switch (xo) {
        case 0:   // fcmpu: unordered compare into selected CR field
        case 32: { // fcmpo: ordered compare (basic compare result)
            const std::uint32_t field = (instruction >> 23) & 7u;
            const double lhs = a;
            const double rhs = b;
            const std::uint32_t result = std::isnan(lhs) || std::isnan(rhs)
                ? 1u : lhs < rhs ? 8u : lhs > rhs ? 4u : 2u;
            const std::uint32_t shift = 28u - field * 4u;
            const std::uint32_t mask = 0xfu << shift;
            state_.cr = (state_.cr & ~mask) | (result << shift);
            break;
        }
        case 12: // frsp: round to single precision, retain in FPR
            write_fpr(static_cast<double>(static_cast<float>(b)));
            break;
        case 22: // fsqrt
            write_fpr(std::sqrt(b));
            break;
        case 23: { // fsel: select FRC for nonnegative FRA, otherwise FRB
            const std::uint32_t selected = a >= -0.0
                ? frc : frb;
            state_.fpr[frt] = state_.fpr[selected];
            break;
        }
        case 40: // fneg
            write_fpr(-b);
            break;
        case 72: // fmr
            state_.fpr[frt] = state_.fpr[frb];
            break;
        case 136: // fnabs
            write_fpr(-std::fabs(b));
            break;
        case 264: // fabs
            write_fpr(std::fabs(b));
            break;
        default:
            return false;
        }
        break;
    }
    case 31: { // X-form integer instructions
        const std::uint32_t rs = rt;
        const std::uint32_t rb = (instruction >> 11) & 31u;
        const std::uint32_t xo = (instruction >> 1) & 0x3ffu;
        const std::uint32_t a = state_.gpr[ra];
        const std::uint32_t b = state_.gpr[rb];
        switch (xo) {
        case 20: { // lwarx: load word and establish a reservation
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            if ((address & 3u) != 0u) {
                throw std::invalid_argument("unaligned lwarx");
            }
            state_.gpr[rt] = memory_.read32_be(address);
            state_.reservation_address = address;
            state_.reservation_valid = true;
            break;
        }
        case 150: { // stwcx.: conditional store and CR0 success indication
            if ((instruction & 1u) == 0u) return false; // Rc must be set.
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            if ((address & 3u) != 0u) {
                throw std::invalid_argument("unaligned stwcx.");
            }
            const bool success = state_.reservation_valid &&
                                 state_.reservation_address == address;
            state_.reservation_valid = false;
            const std::uint32_t so = (state_.xer >> 31) & 1u;
            const std::uint32_t field = (success ? 2u : 0u) | so;
            state_.cr = (state_.cr & 0x0fffffffu) | (field << 28);
            if (success) {
                memory_.write32_be(address, state_.gpr[rs]);
            }
            break;
        }
        case 23: case 55: case 87: case 119:
        case 151: case 183: case 215: case 247:
        case 279: case 311: case 343: case 375:
        case 407: case 439: case 534: case 662:
        case 790: case 918: { // Indexed and byte-reversed memory operations
            const bool update = xo == 55u || xo == 119u || xo == 183u ||
                xo == 247u || xo == 311u || xo == 375u || xo == 439u;
            const bool load = xo == 23u || xo == 55u || xo == 87u ||
                xo == 119u || xo == 279u || xo == 311u || xo == 343u ||
                xo == 375u || xo == 534u || xo == 790u;
            if (update && (ra == 0u || (load ? ra == rt : ra == rs))) return false;
            const std::uint32_t address = (ra == 0u ? 0u : state_.gpr[ra]) + b;
            const bool byte_access = xo == 87u || xo == 119u ||
                                     xo == 215u || xo == 247u;
            const bool half_access = xo == 279u || xo == 311u ||
                xo == 343u || xo == 375u || xo == 407u || xo == 439u ||
                xo == 790u || xo == 918u;
            const bool reverse = xo == 534u || xo == 662u ||
                                 xo == 790u || xo == 918u;
            const std::uint32_t alignment = byte_access ? 1u : half_access ? 2u : 4u;
            if ((address & (alignment - 1u)) != 0u) {
                throw std::invalid_argument("unaligned indexed memory access");
            }
            if (load) {
                std::uint32_t value = 0;
                if (byte_access) {
                    value = memory_.read8(address);
                } else if (half_access) {
                    const std::uint16_t half = memory_.read16_be(address);
                    if (reverse) {
                        value = static_cast<std::uint32_t>((half >> 8) | (half << 8)) & 0xffffu;
                    } else if (xo == 343u || xo == 375u) {
                        value = static_cast<std::uint32_t>(static_cast<std::int32_t>(
                            static_cast<std::int16_t>(half)));
                    } else {
                        value = half;
                    }
                } else {
                    const std::uint32_t word = memory_.read32_be(address);
                    value = reverse ? ((word & 0x000000ffu) << 24) |
                        ((word & 0x0000ff00u) << 8) |
                        ((word & 0x00ff0000u) >> 8) |
                        ((word & 0xff000000u) >> 24) : word;
                }
                state_.gpr[rt] = value;
            } else {
                const std::uint32_t value = state_.gpr[rs];
                invalidate_reservation(address, byte_access ? 1u :
                    half_access ? 2u : 4u);
                if (byte_access) {
                    memory_.write8(address, static_cast<std::uint8_t>(value));
                } else if (half_access) {
                    const std::uint16_t half = static_cast<std::uint16_t>(value);
                    const std::uint16_t stored = reverse
                        ? static_cast<std::uint16_t>((half >> 8) | (half << 8)) : half;
                    memory_.write16_be(address, stored);
                } else {
                    const std::uint32_t stored = reverse
                        ? ((value & 0x000000ffu) << 24) |
                          ((value & 0x0000ff00u) << 8) |
                          ((value & 0x00ff0000u) >> 8) |
                          ((value & 0xff000000u) >> 24) : value;
                    memory_.write32_be(address, stored);
                }
            }
            if (update) state_.gpr[ra] = address;
            break;
        }
        case 512: { // mcrxr: move XER summary bits into a CR field and clear them
            const std::uint32_t field = (instruction >> 23) & 7u;
            const std::uint32_t xer_flags = (state_.xer >> 28) & 0xeu;
            const std::uint32_t shift = 28u - field * 4u;
            const std::uint32_t mask = 0xfu << shift;
            state_.cr = (state_.cr & ~mask) | (xer_flags << shift);
            state_.xer &= ~0xe0000000u;
            break;
        }
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
        case 83: { // mfmsr: move machine-state register to GPR
            state_.gpr[rt] = state_.msr;
            break;
        }
        case 146: { // mtmsr: move GPR to machine-state register
            state_.msr = state_.gpr[rs];
            break;
        }
        case 339: { // mfspr: move supported special-purpose register to GPR
            const std::uint32_t spr = ra | (rb << 5);
            switch (spr) {
            case 1: state_.gpr[rt] = state_.xer; break; // XER
            case 8: state_.gpr[rt] = state_.lr; break;  // LR
            case 9: state_.gpr[rt] = state_.ctr; break; // CTR
            default: return false;
            }
            break;
        }
        case 371: { // mftb: move time-base register to GPR
            const std::uint32_t spr = ra | (rb << 5);
            if (spr == 268u) {
                state_.gpr[rt] = static_cast<std::uint32_t>(state_.time_base);
            } else if (spr == 269u) {
                state_.gpr[rt] = static_cast<std::uint32_t>(state_.time_base >> 32);
            } else {
                return false;
            }
            break;
        }
        case 467: { // mtspr: move GPR to supported special-purpose register
            const std::uint32_t spr = ra | (rb << 5);
            const std::uint32_t value = state_.gpr[rs];
            switch (spr) {
            case 1: state_.xer = value; break; // XER
            case 8: state_.lr = value; break;  // LR
            case 9: state_.ctr = value; break; // CTR
            default: return false;
            }
            break;
        }
        case 598: // sync: memory synchronization barrier
        case 854: // eieio: I/O ordering barrier
        case 566: // tlbsync: translation-lookaside-buffer synchronization
        case 54:  // dcbst: data cache block store
        case 86:  // dcbf: data cache block flush
        case 246: // dcbtst: data cache block touch for store
        case 278: // dcbt: data cache block touch
        case 470: // dcbi: data cache block invalidate
        case 982: // icbi: instruction cache block invalidate
            // No cache/TLB hierarchy is modeled by this interpreter.
            break;
        case 1014: { // dcbz: zero one 32-byte data-cache block
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            const std::uint32_t block = address & ~31u;
            invalidate_reservation(block, 32u);
            for (std::uint32_t offset = 0; offset < 32u; ++offset) {
                memory_.write8(block + offset, 0u);
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
            const std::uint32_t result = (less ? 8u : greater ? 4u : 2u) |
                ((state_.xer >> 31) & 1u);
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
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 8: { // subfc: RB - RA and record no-borrow carry in XER[CA]
            state_.gpr[rt] = b - a;
            state_.xer = (state_.xer & ~(1u << 29)) |
                         (b >= a ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 138: { // adde: add registers and XER[CA]
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(a) + b + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 136: { // subfe: RB - RA - 1 + XER[CA]
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum =
                static_cast<std::uint64_t>(~a) + b + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }

        case 104: // neg: negate RA and write the result to RT
            state_.gpr[rt] = 0u - a;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        case 202: { // addze: RA + XER[CA], recording carry-out
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(a) + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 200: { // subfze: ~RA + XER[CA], recording carry-out
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(~a) + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 232: { // subfme: ~RA + XER[CA] - 1, recording carry-out
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(~a) +
                                      0xffffffffu + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 234: { // addme: RA - 1 + XER[CA], recording carry-out
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(a) +
                                      0xffffffffu + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 266: // add
            state_.gpr[rt] = a + b;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        case 235: // mullw: multiply low 32 bits
            state_.gpr[rt] = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b));
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
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
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 459: { // divwu: unsigned word division
            if (b == 0) {
                state_.gpr[rt] = 0;
            } else {
                state_.gpr[rt] = a / b;
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 75: { // mulhw: signed high word of product
            const std::int64_t product =
                static_cast<std::int64_t>(static_cast<std::int32_t>(a)) *
                static_cast<std::int64_t>(static_cast<std::int32_t>(b));
            state_.gpr[rt] = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(product) >> 32);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 11: { // mulhwu: unsigned high word of product
            const std::uint64_t product =
                static_cast<std::uint64_t>(a) * static_cast<std::uint64_t>(b);
            state_.gpr[rt] = static_cast<std::uint32_t>(product >> 32);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 40: // subf: RT = RB - RA
            state_.gpr[rt] = b - a;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        case 28: // and
            state_.gpr[ra] = state_.gpr[rs] & b;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 444: // or
            state_.gpr[ra] = state_.gpr[rs] | b;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 316: // xor
            state_.gpr[ra] = state_.gpr[rs] ^ b;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 124: // nor
            state_.gpr[ra] = ~(state_.gpr[rs] | b);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 284: // eqv: bitwise equivalence
            state_.gpr[ra] = ~(state_.gpr[rs] ^ b);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 476: // nand
            state_.gpr[ra] = ~(state_.gpr[rs] & b);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 26: { // cntlzw: count leading zero bits
            std::uint32_t value = state_.gpr[rs];
            std::uint32_t count = 0;
            while (count < 32u && (value & 0x80000000u) == 0) {
                ++count;
                value <<= 1;
            }
            state_.gpr[ra] = count;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        }
        case 954: // extsb: sign-extend byte
            state_.gpr[ra] = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(static_cast<std::int8_t>(state_.gpr[rs])));
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 922: // extsh: sign-extend halfword
            state_.gpr[ra] = static_cast<std::uint32_t>(
                static_cast<std::int32_t>(static_cast<std::int16_t>(state_.gpr[rs])));
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 24: // slw: shift left word
            state_.gpr[ra] = (b & 0x20u) != 0 ? 0u : state_.gpr[rs] << (b & 0x3fu);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
            break;
        case 536: // srw: shift right word
            state_.gpr[ra] = (b & 0x20u) != 0 ? 0u : state_.gpr[rs] >> (b & 0x3fu);
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
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
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
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
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[ra]);
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
