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

template <typename State, typename Operation>
auto record_dar_on_fault(State& state, std::uint32_t address,
                         Operation&& operation) -> decltype(operation()) {
    try {
        return operation();
    } catch (const std::out_of_range&) {
        state.dar = address;
        throw;
    }
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

bool BroadwayCpu::enter_exception() noexcept {
    if (state_.exception == Exception::None) return false;

    // PowerPC 750-family synchronous exception vector offsets.
    std::uint32_t vector = 0;
    switch (state_.exception) {
    case Exception::Alignment:            vector = 0x0600u; break;
    case Exception::InstructionStorage:   vector = 0x0400u; break;
    case Exception::DataStorage:          vector = 0x0300u; break;
    case Exception::IllegalInstruction:
    case Exception::PrivilegedInstruction: vector = 0x0700u; break;
    case Exception::SystemCall:           vector = 0x0c00u; break;
    case Exception::FloatingPointUnavailable: vector = 0x0800u; break;
    case Exception::None: return false;
    }

    // Save the faulting instruction address and pre-exception MSR.
    state_.srr0 = state_.pc;
    state_.srr1 = state_.msr;

    // Exception entry clears translation, privilege, external interrupt,
    // floating-point availability and recoverability state. The handler
    // starts in supervisor mode; implementation-specific MSR bits are kept
    // conservative rather than synthesizing unsupported state.
    constexpr std::uint32_t msr_pow = 1u << (31u - 18u);
    constexpr std::uint32_t msr_ee  = 1u << (31u - 15u);
    constexpr std::uint32_t msr_pr  = 1u << (31u - 14u);
    constexpr std::uint32_t msr_fp  = 1u << (31u - 13u);
    constexpr std::uint32_t msr_fe0 = 1u << (31u - 11u);
    constexpr std::uint32_t msr_be  = 1u << (31u - 10u);
    constexpr std::uint32_t msr_se  = 1u << (31u - 9u);
    constexpr std::uint32_t msr_fe1 = 1u << (31u - 8u);
    constexpr std::uint32_t msr_ir  = 1u << (31u - 5u);
    constexpr std::uint32_t msr_dr  = 1u << (31u - 4u);
    constexpr std::uint32_t msr_ri  = 1u << (31u - 1u);
    constexpr std::uint32_t msr_le  = 1u;
    state_.msr &= ~(msr_pow | msr_ee | msr_pr | msr_fp | msr_fe0 | msr_be |
                    msr_se | msr_fe1 | msr_ir | msr_dr | msr_ri | msr_le);

    // The high-vector prefix is selected by MSR[IP].
    constexpr std::uint32_t msr_ip = 1u << (31u - 6u);
    const std::uint32_t prefix = (state_.srr1 & msr_ip) ? 0xfff00000u : 0u;
    state_.pc = prefix + vector;
    state_.exception = Exception::None;
    state_.reservation_valid = false;
    return true;
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
        if (state_.exception == Exception::None) {
            state_.exception = Exception::IllegalInstruction;
        }
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
    const auto read8 = [&](std::uint32_t address) {
        return record_dar_on_fault(state_, address, [&] { return memory_.read8(address); });
    };
    const auto read16 = [&](std::uint32_t address) {
        return record_dar_on_fault(state_, address, [&] { return memory_.read16_be(address); });
    };
    const auto read32 = [&](std::uint32_t address) {
        return record_dar_on_fault(state_, address, [&] { return memory_.read32_be(address); });
    };
    const auto read64 = [&](std::uint32_t address) {
        return record_dar_on_fault(state_, address, [&] { return memory_.read64_be(address); });
    };
    const auto write8 = [&](std::uint32_t address, std::uint8_t value) {
        record_dar_on_fault(state_, address, [&] { memory_.write8(address, value); });
    };
    const auto write16 = [&](std::uint32_t address, std::uint16_t value) {
        record_dar_on_fault(state_, address, [&] { memory_.write16_be(address, value); });
    };
    const auto write32 = [&](std::uint32_t address, std::uint32_t value) {
        record_dar_on_fault(state_, address, [&] { memory_.write32_be(address, value); });
    };
    const auto write64 = [&](std::uint32_t address, std::uint64_t value) {
        record_dar_on_fault(state_, address, [&] { memory_.write64_be(address, value); });
    };

    const std::uint32_t instruction = memory_.read32_be(state_.pc);
    const std::uint32_t opcode = instruction >> 26;
    const std::uint32_t rt = (instruction >> 21) & 31u;
    const std::uint32_t ra = (instruction >> 16) & 31u;
    const std::uint32_t immediate = instruction & 0xffffu;
    const std::uint32_t next_pc = state_.pc + 4u;

    // Floating-point instructions trap before execution when MSR[FP] is clear.
    constexpr std::uint32_t msr_fp_enable = 1u << (31u - 13u);
    bool uses_fpu = opcode == 48u || opcode == 49u || opcode == 50u ||
        opcode == 51u || opcode == 52u || opcode == 53u ||
        opcode == 54u || opcode == 55u || opcode == 59u || opcode == 63u;
    if (opcode == 31u) {
        const std::uint32_t xo = (instruction >> 1) & 0x3ffu;
        uses_fpu = xo == 535u || xo == 567u || xo == 599u || xo == 631u ||
            xo == 663u || xo == 695u || xo == 727u || xo == 759u ||
            xo == 983u;
    }
    if (uses_fpu && (state_.msr & msr_fp_enable) == 0u) {
        state_.exception = Exception::FloatingPointUnavailable;
        return false;
    }
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
    case 17: { // sc: system call
        if (instruction != 0x44000002u) return false;
        state_.pc = next_pc;
        state_.exception = Exception::SystemCall;
        return false;
    }
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
            state_.dar = address;
            throw std::invalid_argument("unaligned floating-point load");
        }
        if (is_double) {
            state_.fpr[frt] = read64(address);
        } else {
            const float value = std::bit_cast<float>(read32(address));
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
            state_.dar = address;
            throw std::invalid_argument("unaligned floating-point store");
        }
        const double value = std::bit_cast<double>(state_.fpr[frs]);
        invalidate_reservation(address, is_double ? 8u : 4u);
        if (is_double) {
            write64(address, state_.fpr[frs]);
        } else {
            write32(address, std::bit_cast<std::uint32_t>(
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
        if (xo == 50u) { // rfi: privileged return from interrupt
            if (instruction != 0x4c000064u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            if ((state_.srr0 & 3u) != 0u) {
                state_.exception = Exception::Alignment;
                return false;
            }
            state_.msr = state_.srr1;
            state_.pc = state_.srr0;
            state_.reservation_valid = false;
            return true;
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
        if (update && (ra == 0u || ra == rt)) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((address & 3u) != 0u) { state_.dar = address; throw std::invalid_argument("unaligned lwz"); }
        const std::uint32_t value = read32(address);
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
        if (update && (ra == 0u || ra == rt)) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((opcode >= 40u) && (address & 1u) != 0u) {
            state_.dar = address;
            throw std::invalid_argument("unaligned halfword load");
        }
        std::uint32_t value = 0;
        if (opcode == 34u || opcode == 35u) {
            value = read8(address);
        } else {
            const std::uint16_t half = read16(address);
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
        if (update && ra == 0u) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        const bool halfword = opcode == 44u || opcode == 45u;
        if (halfword && (address & 1u) != 0u) {
            state_.dar = address;
            throw std::invalid_argument("unaligned halfword store");
        }
        invalidate_reservation(address, halfword ? 2u : 1u);
        if (halfword) write16(address,
            static_cast<std::uint16_t>(state_.gpr[rt]));
        else write8(address, static_cast<std::uint8_t>(state_.gpr[rt]));
        if (update) state_.gpr[ra] = address;
        break;
    }
    case 46: // lmw: load multiple words
    case 47: { // stmw: store multiple words
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((address & 3u) != 0u) {
            state_.dar = address;
            throw std::invalid_argument("unaligned multiple-word access");
        }
        // Validate the complete range before changing registers or memory.
        const std::size_t word_count = 32u - rt;
        const std::size_t byte_count = word_count * 4u;
        if (!memory_.contains(address, byte_count)) {
            state_.dar = address;
            throw std::out_of_range("multiple-word access outside memory");
        }
        if (opcode == 46u) {
            for (std::uint32_t reg = rt; reg < 32u; ++reg) {
                state_.gpr[reg] = read32(
                    address + (reg - rt) * 4u);
            }
        } else {
            invalidate_reservation(address, static_cast<std::uint32_t>(byte_count));
            for (std::uint32_t reg = rt; reg < 32u; ++reg) {
                write32(address + (reg - rt) * 4u,
                                   state_.gpr[reg]);
            }
        }
        break;
    }
    case 36: // stw
    case 37: { // stwu: store word and update RA
        const bool update = opcode == 37u;
        if (update && ra == 0u) return false;
        const std::uint32_t address = base + static_cast<std::uint32_t>(
            sign_extend_16(immediate));
        if ((address & 3u) != 0u) { state_.dar = address; throw std::invalid_argument("unaligned stw"); }
        invalidate_reservation(address, 4u);
        write32(address, state_.gpr[rt]);
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
        auto update_fpu_cr1 = [&]() {
            if ((instruction & 1u) != 0u) {
                const std::uint32_t cr1 = (state_.fpscr >> 28) & 0xfu;
                state_.cr = (state_.cr & 0x00ffffffu) | (cr1 << 24);
            }
        };
        constexpr std::uint32_t fpscr_invalid_causes =
            (1u << 24) | (1u << 23) | (1u << 22) |
            (1u << 21) | (1u << 20) | (1u << 19) |
            (1u << 10) | (1u << 9) | (1u << 8);
        constexpr std::uint32_t fpscr_ox = 1u << 28;
        constexpr std::uint32_t fpscr_ux = 1u << 27;
        constexpr std::uint32_t fpscr_zx = 1u << 26;
        constexpr std::uint32_t fpscr_xx = 1u << 25;
        constexpr std::uint32_t fpscr_fx = 1u << 31;
        constexpr std::uint32_t fpscr_fex = 1u << 30;
        constexpr std::uint32_t fpscr_vx = 1u << 29;
        auto refresh_fpscr_summaries = [&]() {
            const bool invalid = (state_.fpscr & fpscr_invalid_causes) != 0u;
            if (invalid) state_.fpscr |= fpscr_vx;
            else state_.fpscr &= ~fpscr_vx;

            constexpr std::uint32_t ve = 1u << 7;
            constexpr std::uint32_t oe = 1u << 6;
            constexpr std::uint32_t ue = 1u << 5;
            constexpr std::uint32_t ze = 1u << 4;
            constexpr std::uint32_t xe = 1u << 3;
            const bool enabled =
                (invalid && (state_.fpscr & ve) != 0u) ||
                ((state_.fpscr & fpscr_ox) != 0u && (state_.fpscr & oe) != 0u) ||
                ((state_.fpscr & fpscr_ux) != 0u && (state_.fpscr & ue) != 0u) ||
                ((state_.fpscr & fpscr_zx) != 0u && (state_.fpscr & ze) != 0u) ||
                ((state_.fpscr & fpscr_xx) != 0u && (state_.fpscr & xe) != 0u);
            if (enabled) state_.fpscr |= fpscr_fex;
            else state_.fpscr &= ~fpscr_fex;
        };
        auto raise_fpscr_exception = [&](std::uint32_t flags) {
            constexpr std::uint32_t exception_status =
                fpscr_invalid_causes | fpscr_ox | fpscr_ux |
                fpscr_zx | fpscr_xx;
            const std::uint32_t old_status = state_.fpscr & exception_status;
            state_.fpscr |= flags;
            const std::uint32_t new_status = state_.fpscr & exception_status;
            if ((new_status & ~old_status) != 0u) state_.fpscr |= fpscr_fx;
            refresh_fpscr_summaries();
        };
        auto is_signaling_nan = [](std::uint64_t bits) {
            constexpr std::uint64_t exponent = 0x7ff0000000000000ull;
            constexpr std::uint64_t fraction = 0x000fffffffffffffull;
            constexpr std::uint64_t quiet = 0x0008000000000000ull;
            return (bits & exponent) == exponent &&
                   (bits & fraction) != 0u && (bits & quiet) == 0u;
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
            const bool uses_frc = a_xo == 25u || a_xo >= 28u;
            const bool signaling_operand = is_signaling_nan(state_.fpr[fra]) ||
                (uses_frc ? is_signaling_nan(state_.fpr[frc])
                          : is_signaling_nan(state_.fpr[frb])) ||
                (a_xo >= 28u && is_signaling_nan(state_.fpr[frb]));
            std::uint32_t arithmetic_exceptions = signaling_operand
                ? (1u << 24) : 0u; // VXSNAN
            const double multiply_rhs = uses_frc ? c_value : b;
            switch (a_xo) {
            case 18: // division
                if (a == 0.0 && b == 0.0)
                    arithmetic_exceptions |= 1u << 21; // VXZDZ
                else if (std::isinf(a) && std::isinf(b))
                    arithmetic_exceptions |= 1u << 22; // VXIDI
                else if (std::isfinite(a) && a != 0.0 && b == 0.0)
                    arithmetic_exceptions |= fpscr_zx; // ZX
                break;
            case 20: // subtraction
                if (std::isinf(a) && std::isinf(b) &&
                    std::signbit(a) == std::signbit(b))
                    arithmetic_exceptions |= 1u << 23; // VXISI
                break;
            case 21: // addition
                if (std::isinf(a) && std::isinf(b) &&
                    std::signbit(a) != std::signbit(b))
                    arithmetic_exceptions |= 1u << 23; // VXISI
                break;
            case 25: // multiplication
                if ((a == 0.0 && std::isinf(multiply_rhs)) ||
                    (std::isinf(a) && multiply_rhs == 0.0))
                    arithmetic_exceptions |= 1u << 20; // VXIMZ
                break;
            default:
                break;
            }
            if (arithmetic_exceptions != 0u)
                raise_fpscr_exception(arithmetic_exceptions);
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
            update_fpu_cr1();
            break;
        }
        if (opcode == 59u && a_xo == 24u) { // fres: reciprocal estimate (deterministic approximation)
            const float value = static_cast<float>(b);
            const float result = 1.0f / value;
            write_fpr(static_cast<double>(result));
            update_fpu_cr1();
            break;
        }
        if (opcode == 59u) return false;
        switch (xo) {
        case 0:   // fcmpu: unordered compare into selected CR field
        case 32: { // fcmpo: ordered compare (basic compare result)
            const std::uint32_t field = (instruction >> 23) & 7u;
            const double lhs = a;
            const double rhs = b;
            const bool unordered = std::isnan(lhs) || std::isnan(rhs);
            const std::uint32_t result = unordered
                ? 1u : lhs < rhs ? 8u : lhs > rhs ? 4u : 2u;
            const std::uint32_t shift = 28u - field * 4u;
            const std::uint32_t mask = 0xfu << shift;
            state_.cr = (state_.cr & ~mask) | (result << shift);

            // FPCC (FPSCR[12:15]) mirrors the comparison classification.
            state_.fpscr = (state_.fpscr & ~0x0000f000u) | (result << 12);
            const bool signaling_nan =
                is_signaling_nan(state_.fpr[fra]) ||
                is_signaling_nan(state_.fpr[frb]);
            const bool invalid_ordered_compare = unordered && xo == 32u;
            std::uint32_t exception_flags = 0u;
            if (signaling_nan) exception_flags |= 1u << 24; // VXSNAN
            if (invalid_ordered_compare) exception_flags |= 1u << 19; // VXVC
            if (exception_flags != 0u) raise_fpscr_exception(exception_flags);
            break;
        }
        case 64: { // mcrfs: move an FPSCR field to a CR field
            if ((instruction & 1u) != 0u) return false;
            const std::uint32_t bf = (instruction >> 23) & 7u;
            const std::uint32_t bfa = (instruction >> 18) & 7u;
            const std::uint32_t source_shift = 28u - bfa * 4u;
            const std::uint32_t destination_shift = 28u - bf * 4u;
            const std::uint32_t field = (state_.fpscr >> source_shift) & 0xfu;
            const std::uint32_t mask = 0xfu << destination_shift;
            state_.cr = (state_.cr & ~mask) | (field << destination_shift);
            break;
        }
        case 134: { // mtfsfi: move immediate to one FPSCR field
            const std::uint32_t field = (instruction >> 23) & 7u;
            const std::uint32_t immediate = (instruction >> 12) & 0xfu;
            const std::uint32_t shift = 28u - field * 4u;
            const std::uint32_t mask = 0xfu << shift;
            state_.fpscr = (state_.fpscr & ~mask) | (immediate << shift);
            refresh_fpscr_summaries();
            if ((instruction & 1u) != 0u) {
                const std::uint32_t cr1 = (state_.fpscr >> 28) & 0xfu;
                state_.cr = (state_.cr & 0x00ffffffu) | (cr1 << 24);
            }
            break;
        }
        case 38:  // mtfsb1: set selected FPSCR bit
        case 70: { // mtfsb0: clear selected FPSCR bit
            const std::uint32_t bit = (instruction >> 21) & 31u;
            const std::uint32_t mask = 1u << (31u - bit);
            if (xo == 38u) state_.fpscr |= mask;
            else state_.fpscr &= ~mask;
            refresh_fpscr_summaries();
            if ((instruction & 1u) != 0u) {
                const std::uint32_t cr1 = (state_.fpscr >> 28) & 0xfu;
                state_.cr = (state_.cr & 0x0fffffffu) | (cr1 << 24);
            }
            break;
        }
        case 583: // mffs: move FPSCR to FPR
            state_.fpr[frt] = 0xfff8000000000000ull | state_.fpscr;
            break;
        case 711: { // mtfsf: move selected FPR nibbles to FPSCR
            const std::uint32_t field_mask = (instruction >> 17) & 0xffu;
            const std::uint32_t source = static_cast<std::uint32_t>(state_.fpr[frb]);
            for (std::uint32_t field = 0; field < 8; ++field) {
                if ((field_mask & (1u << (7u - field))) == 0u) continue;
                const std::uint32_t shift = 28u - field * 4u;
                const std::uint32_t mask = 0xfu << shift;
                state_.fpscr = (state_.fpscr & ~mask) | (source & mask);
            }
            refresh_fpscr_summaries();
            break;
        }
        case 14: // fctiw: convert to signed word using current rounding mode
        case 15: { // fctiwz: convert to signed word toward zero
            const double source = b;
            std::int32_t converted = std::numeric_limits<std::int32_t>::min();
            bool invalid_conversion = !std::isfinite(source);
            if (std::isfinite(source)) {
                double rounded = std::trunc(source);
                if (xo == 14u) {
                    switch (state_.fpscr & 3u) {
                    case 0u: rounded = std::nearbyint(source); break; // nearest
                    case 1u: rounded = std::trunc(source); break;    // toward zero
                    case 2u: rounded = std::ceil(source); break;     // toward +infinity
                    case 3u: rounded = std::floor(source); break;    // toward -infinity
                    }
                }
                if (rounded >= static_cast<double>(std::numeric_limits<std::int32_t>::min()) &&
                    rounded <= static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
                    converted = static_cast<std::int32_t>(rounded);
                    invalid_conversion = false;
                } else {
                    invalid_conversion = true;
                }
            }
            if (invalid_conversion) raise_fpscr_exception(1u << 8); // VXCVI
            state_.fpr[frt] = 0xfff8000000000000ull |
                static_cast<std::uint32_t>(converted);
            break;
        }
        case 12: // frsp: round to single precision, retain in FPR
            write_fpr(static_cast<double>(static_cast<float>(b)));
            break;
        case 22: // fsqrt
            if (std::isless(b, 0.0))
                raise_fpscr_exception(1u << 9); // VXSQRT
            write_fpr(std::sqrt(b));
            break;
        case 26: // frsqrte: reciprocal square-root estimate
            write_fpr(1.0 / std::sqrt(b));
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
        update_fpu_cr1();
        break;
    }
    case 31: { // X-form integer instructions
        const std::uint32_t rs = rt;
        const std::uint32_t rb = (instruction >> 11) & 31u;
        const std::uint32_t xo = (instruction >> 1) & 0x3ffu;
        const std::uint32_t a = state_.gpr[ra];
        const std::uint32_t b = state_.gpr[rb];
        const auto set_overflow = [&](bool overflow) {
            state_.xer = (state_.xer & ~(1u << 30)) |
                         (overflow ? (1u << 30) : 0u);
            if (overflow) state_.xer |= 1u << 31;
        };
        switch (xo) {
        case 535: case 567: case 599: case 631:
        case 663: case 695: case 727: case 759: { // Indexed floating-point loads/stores
            const bool load = xo == 535u || xo == 567u ||
                              xo == 599u || xo == 631u;
            const bool update = xo == 567u || xo == 631u ||
                                xo == 695u || xo == 759u;
            const bool is_double = xo == 599u || xo == 631u ||
                                   xo == 727u || xo == 759u;
            if (update && ra == 0u) return false;
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            const std::uint32_t alignment = is_double ? 8u : 4u;
            if ((address & (alignment - 1u)) != 0u) {
                state_.dar = address;
                throw std::invalid_argument("unaligned indexed floating-point access");
            }
            if (load) {
                if (is_double) {
                    state_.fpr[rt] = read64(address);
                } else {
                    const float value = std::bit_cast<float>(read32(address));
                    state_.fpr[rt] = std::bit_cast<std::uint64_t>(
                        static_cast<double>(value));
                }
            } else {
                const double value = std::bit_cast<double>(state_.fpr[rs]);
                invalidate_reservation(address, is_double ? 8u : 4u);
                if (is_double) {
                    write64(address, state_.fpr[rs]);
                } else {
                    write32(address, std::bit_cast<std::uint32_t>(
                        static_cast<float>(value)));
                }
            }
            if (update) state_.gpr[ra] = address;
            break;
        }
        case 983: { // stfiwx: store the low-order word of an FPR
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            if ((address & 3u) != 0u) {
                state_.dar = address;
                throw std::invalid_argument("unaligned stfiwx");
            }
            invalidate_reservation(address, 4u);
            write32(address,
                static_cast<std::uint32_t>(state_.fpr[rs]));
            break;
        }
        case 20: { // lwarx: load word and establish a reservation
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            if ((address & 3u) != 0u) {
                state_.dar = address;
                throw std::invalid_argument("unaligned lwarx");
            }
            state_.gpr[rt] = read32(address);
            state_.reservation_address = address;
            state_.reservation_valid = true;
            break;
        }
        case 150: { // stwcx.: conditional store and CR0 success indication
            if ((instruction & 1u) == 0u) return false; // Rc must be set.
            const std::uint32_t address = (ra == 0u ? 0u : a) + b;
            if ((address & 3u) != 0u) {
                state_.dar = address;
                throw std::invalid_argument("unaligned stwcx.");
            }
            const bool success = state_.reservation_valid &&
                                 state_.reservation_address == address;
            state_.reservation_valid = false;
            const std::uint32_t so = (state_.xer >> 31) & 1u;
            const std::uint32_t field = (success ? 2u : 0u) | so;
            state_.cr = (state_.cr & 0x0fffffffu) | (field << 28);
            if (success) {
                write32(address, state_.gpr[rs]);
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
            if (update && (ra == 0u || (load && ra == rt))) return false;
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
                state_.dar = address;
                throw std::invalid_argument("unaligned indexed memory access");
            }
            if (load) {
                std::uint32_t value = 0;
                if (byte_access) {
                    value = read8(address);
                } else if (half_access) {
                    const std::uint16_t half = read16(address);
                    if (reverse) {
                        value = static_cast<std::uint32_t>((half >> 8) | (half << 8)) & 0xffffu;
                    } else if (xo == 343u || xo == 375u) {
                        value = static_cast<std::uint32_t>(static_cast<std::int32_t>(
                            static_cast<std::int16_t>(half)));
                    } else {
                        value = half;
                    }
                } else {
                    const std::uint32_t word = read32(address);
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
                    write8(address, static_cast<std::uint8_t>(value));
                } else if (half_access) {
                    const std::uint16_t half = static_cast<std::uint16_t>(value);
                    const std::uint16_t stored = reverse
                        ? static_cast<std::uint16_t>((half >> 8) | (half << 8)) : half;
                    write16(address, stored);
                } else {
                    const std::uint32_t stored = reverse
                        ? ((value & 0x000000ffu) << 24) |
                          ((value & 0x0000ff00u) << 8) |
                          ((value & 0x00ff0000u) >> 8) |
                          ((value & 0xff000000u) >> 24) : value;
                    write32(address, stored);
                }
            }
            if (update) state_.gpr[ra] = address;
            break;
        }
        case 778: { // addo: signed add with overflow enabled
            const std::uint32_t result = a + b;
            state_.gpr[rt] = result;
            const bool overflow = ((~(a ^ b) & (a ^ result)) >> 31) != 0u;
            set_overflow(overflow);
            if ((instruction & 1u) != 0u) update_cr0(state_, result);
            break;
        }
        case 552: { // subfo: RB - RA with signed overflow enabled
            const std::uint32_t result = b - a;
            state_.gpr[rt] = result;
            const bool overflow = (((b ^ a) & (b ^ result)) >> 31) != 0u;
            set_overflow(overflow);
            if ((instruction & 1u) != 0u) update_cr0(state_, result);
            break;
        }
        case 747: { // mullwo: signed multiply low word with overflow enabled
            const std::int64_t product =
                static_cast<std::int64_t>(static_cast<std::int32_t>(a)) *
                static_cast<std::int64_t>(static_cast<std::int32_t>(b));
            const std::int32_t result = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(product));
            state_.gpr[rt] = static_cast<std::uint32_t>(result);
            set_overflow(product < std::numeric_limits<std::int32_t>::min() ||
                         product > std::numeric_limits<std::int32_t>::max());
            if ((instruction & 1u) != 0u) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 1003: { // divwo: signed divide with overflow enabled
            const std::int32_t dividend = static_cast<std::int32_t>(a);
            const std::int32_t divisor = static_cast<std::int32_t>(b);
            const bool overflow = divisor == 0 ||
                (dividend == std::numeric_limits<std::int32_t>::min() &&
                 divisor == -1);
            state_.gpr[rt] = overflow ? 0u :
                static_cast<std::uint32_t>(dividend / divisor);
            set_overflow(overflow);
            if ((instruction & 1u) != 0u) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 616: { // nego: negate with signed overflow enabled
            state_.gpr[rt] = 0u - a;
            set_overflow(a == 0x80000000u);
            if ((instruction & 1u) != 0u) update_cr0(state_, state_.gpr[rt]);
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
            if ((instruction & 1u) != 0u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            state_.gpr[rt] = state_.msr;
            break;
        }
        case 146: { // mtmsr: move GPR to machine-state register
            if ((instruction & 1u) != 0u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            state_.msr = state_.gpr[rs];
            break;
        }
        case 339: { // mfspr: move supported special-purpose register to GPR
            if ((instruction & 1u) != 0u) return false;
            const std::uint32_t spr = ra | (rb << 5);
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u && spr != 1u && spr != 8u && spr != 9u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            switch (spr) {
            case 1: state_.gpr[rt] = state_.xer; break; // XER
            case 8: state_.gpr[rt] = state_.lr; break;  // LR
            case 9: state_.gpr[rt] = state_.ctr; break; // CTR
            case 18: state_.gpr[rt] = state_.dsisr; break; // DSISR
            case 19: state_.gpr[rt] = state_.dar; break; // DAR
            case 22: state_.gpr[rt] = state_.dec; break; // DEC
            case 25: state_.gpr[rt] = state_.sdr1; break; // SDR1
            case 282: state_.gpr[rt] = state_.ear; break; // EAR
            case 26: state_.gpr[rt] = state_.srr0; break; // SRR0
            case 27: state_.gpr[rt] = state_.srr1; break; // SRR1
            case 272: case 273: case 274: case 275:
                state_.gpr[rt] = state_.sprg[spr - 272u]; break;
            default: return false;
            }
            break;
        }
        case 371: { // mftb: move time-base register to GPR
            if ((instruction & 1u) != 0u) return false;
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
            if ((instruction & 1u) != 0u) return false;
            const std::uint32_t spr = ra | (rb << 5);
            const std::uint32_t value = state_.gpr[rs];
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u && spr != 1u && spr != 8u && spr != 9u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            switch (spr) {
            case 1: state_.xer = value; break; // XER
            case 8: state_.lr = value; break;  // LR
            case 9: state_.ctr = value; break; // CTR
            case 18: state_.dsisr = value; break; // DSISR
            case 19: state_.dar = value; break; // DAR
            case 22: state_.dec = value; break; // DEC
            case 25: state_.sdr1 = value; break; // SDR1
            case 282: state_.ear = value; break; // EAR
            case 26: state_.srr0 = value; break; // SRR0
            case 27: state_.srr1 = value; break; // SRR1
            case 272: case 273: case 274: case 275:
                state_.sprg[spr - 272u] = value; break;
            default: return false;
            }
            break;
        }
        case 595: { // mfsr: move segment register to GPR
            if ((instruction & 0x0010f801u) != 0u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            state_.gpr[rt] = state_.segment_registers[ra & 0x0fu];
            break;
        }
        case 210: { // mtsr: move GPR to segment register
            if ((instruction & 0x0010f801u) != 0u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            state_.segment_registers[ra & 0x0fu] = state_.gpr[rs];
            break;
        }
        case 659: { // mfsrin: move indexed segment register to GPR
            if ((instruction & 0x001f0001u) != 0u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            const std::uint32_t segment = state_.gpr[rb] >> 28;
            state_.gpr[rt] = state_.segment_registers[segment];
            break;
        }
        case 242: { // mtsrin: move GPR to indexed segment register
            if ((instruction & 0x001f0001u) != 0u) return false;
            constexpr std::uint32_t msr_pr = 1u << (31u - 14u);
            if ((state_.msr & msr_pr) != 0u) {
                state_.exception = Exception::PrivilegedInstruction;
                return false;
            }
            const std::uint32_t segment = state_.gpr[rb] >> 28;
            state_.segment_registers[segment] = state_.gpr[rs];
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
                write8(block + offset, 0u);
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
        case 10: case 522: { // addc / addco
            const std::uint64_t sum = static_cast<std::uint64_t>(a) + b;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 522u) {
                const std::int64_t wide = static_cast<std::int64_t>(static_cast<std::int32_t>(a)) + static_cast<std::int32_t>(b);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 8: case 520: { // subfc / subfco
            state_.gpr[rt] = b - a;
            state_.xer = (state_.xer & ~(1u << 29)) |
                         (b >= a ? (1u << 29) : 0u);
            if (xo == 520u) {
                const std::int64_t wide = static_cast<std::int64_t>(static_cast<std::int32_t>(b)) - static_cast<std::int32_t>(a);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 138: case 650: { // adde / addeo
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(a) + b + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 650u) {
                const std::int64_t wide = static_cast<std::int64_t>(static_cast<std::int32_t>(a)) + static_cast<std::int32_t>(b) + static_cast<std::int64_t>(carry_in);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 136: case 648: { // subfe / subfeo
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum =
                static_cast<std::uint64_t>(~a) + b + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 648u) {
                const std::int64_t wide = static_cast<std::int64_t>(static_cast<std::int32_t>(b)) - static_cast<std::int32_t>(a) - 1 + static_cast<std::int64_t>(carry_in);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }

        case 104: // neg: negate RA and write the result to RT
            state_.gpr[rt] = 0u - a;
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        case 202: case 714: { // addze / addzeo
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(a) + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 714u) {
                const std::int64_t wide = static_cast<std::int64_t>(static_cast<std::int32_t>(a)) + static_cast<std::int64_t>(carry_in);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 200: case 712: { // subfze / subfzeo
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(~a) + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 712u) {
                const std::int64_t wide = -static_cast<std::int64_t>(static_cast<std::int32_t>(a)) - 1 + static_cast<std::int64_t>(carry_in);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 232: case 744: { // subfme / subfmeo
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(~a) +
                                      0xffffffffu + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 744u) {
                const std::int64_t wide = -static_cast<std::int64_t>(static_cast<std::int32_t>(a)) - 2 + static_cast<std::int64_t>(carry_in);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
            if ((instruction & 1u) != 0) update_cr0(state_, state_.gpr[rt]);
            break;
        }
        case 234: case 746: { // addme / addmeo
            const std::uint64_t carry_in = (state_.xer >> 29) & 1u;
            const std::uint64_t sum = static_cast<std::uint64_t>(a) +
                                      0xffffffffu + carry_in;
            state_.gpr[rt] = static_cast<std::uint32_t>(sum);
            state_.xer = (state_.xer & ~(1u << 29)) |
                         ((sum >> 32) != 0 ? (1u << 29) : 0u);
            if (xo == 746u) {
                const std::int64_t wide = static_cast<std::int64_t>(static_cast<std::int32_t>(a)) - 1 + static_cast<std::int64_t>(carry_in);
                set_overflow(wide < INT32_MIN || wide > INT32_MAX);
            }
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
            // PowerPC sets CA only when a negative operand loses at least
            // one non-sign 1-bit during the arithmetic right shift.
            const bool negative = (value & 0x80000000u) != 0u;
            const bool shifted_out_ones = negative && (shift >= 32u
                ? (value & 0x7fffffffu) != 0u
                : shift != 0u && (value & ((1u << shift) - 1u)) != 0u);
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
            const bool negative = (value & 0x80000000u) != 0u;
            const bool shifted_out_ones = negative && shift != 0u &&
                (value & ((1u << shift) - 1u)) != 0u;
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
