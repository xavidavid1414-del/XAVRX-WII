#include "xavrx/broadway_cpu.hpp"

#include <cstdint>

namespace xavrx {
namespace {
std::int32_t sign_extend_16(std::uint32_t value) noexcept {
    return static_cast<std::int16_t>(value & 0xffffu);
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
    default:
        return false;
    }

    state_.pc = next_pc;
    return true;
}

} // namespace xavrx
