#include "xavrx/cpu/executor.hpp"
#include "xavrx/cpu/decoder.hpp"
#include <bit>
#include <cstdint>

namespace xavrx::cpu {
namespace {
constexpr std::uint32_t field(std::uint32_t word, unsigned shift) noexcept {
    return (word >> shift) & 0x1FU;
}
constexpr std::uint16_t immediate(std::uint32_t word) noexcept {
    return static_cast<std::uint16_t>(word & 0xFFFFU);
}
constexpr std::int32_t signed_immediate(std::uint32_t word) noexcept {
    const auto value = static_cast<std::uint32_t>(immediate(word));
    return value < 0x8000U ? static_cast<std::int32_t>(value)
                           : static_cast<std::int32_t>(value) - 65536;
}
void update_cr0(CpuState& state, std::uint32_t value) noexcept {
    const auto signed_value = std::bit_cast<std::int32_t>(value);
    const auto flags = static_cast<std::uint8_t>(
        (signed_value < 0 ? 8U : 0U) |
        (signed_value > 0 ? 4U : 0U) |
        (signed_value == 0 ? 2U : 0U) |
        ((state.xer() >> 31U) & 1U));
    state.set_condition_field(0, flags);
}
constexpr bool record_form(std::uint32_t word) noexcept {
    return (word & 1U) != 0U;
}
}

ExecutionResult IntegerExecutor::execute(std::uint32_t word, CpuState& state) noexcept {
    const auto decoded = InstructionDecoder::decode(word);
    if (!decoded.valid()) return ExecutionResult::InvalidInstruction;

    const auto rt = field(word, 21U);
    const auto ra = field(word, 16U);
    const auto rb = field(word, 11U);
    const auto rs = field(word, 21U);
    std::uint32_t result = 0U;
    bool write_result = false;
    bool update_condition = false;
    bool logical_destination_ra = false;

    switch (decoded.id) {
    case InstructionId::Addi:
        result = (ra == 0U ? 0U : state.gpr(ra)) +
                 static_cast<std::uint32_t>(signed_immediate(word));
        write_result = true;
        break;
    case InstructionId::Addis:
        result = (ra == 0U ? 0U : state.gpr(ra)) +
                 (static_cast<std::uint32_t>(signed_immediate(word)) << 16U);
        write_result = true;
        break;
    case InstructionId::Ori:
        result = state.gpr(rs) | static_cast<std::uint32_t>(immediate(word));
        write_result = logical_destination_ra = true;
        break;
    case InstructionId::Oris:
        result = state.gpr(rs) | (static_cast<std::uint32_t>(immediate(word)) << 16U);
        write_result = logical_destination_ra = true;
        break;
    case InstructionId::Xori:
        result = state.gpr(rs) ^ static_cast<std::uint32_t>(immediate(word));
        write_result = logical_destination_ra = true;
        break;
    case InstructionId::Xoris:
        result = state.gpr(rs) ^ (static_cast<std::uint32_t>(immediate(word)) << 16U);
        write_result = logical_destination_ra = true;
        break;
    case InstructionId::AndiDot:
        result = state.gpr(rs) & static_cast<std::uint32_t>(immediate(word));
        write_result = logical_destination_ra = true;
        update_condition = true;
        break;
    case InstructionId::Add:
        if ((word & (1U << 10U)) != 0U) return ExecutionResult::UnsupportedEncoding;
        result = state.gpr(ra) + state.gpr(rb);
        write_result = true;
        update_condition = record_form(word);
        break;
    case InstructionId::Subf:
        if ((word & (1U << 10U)) != 0U) return ExecutionResult::UnsupportedEncoding;
        result = state.gpr(rb) - state.gpr(ra);
        write_result = true;
        update_condition = record_form(word);
        break;
    case InstructionId::And:
        result = state.gpr(rs) & state.gpr(rb);
        write_result = logical_destination_ra = true;
        update_condition = record_form(word);
        break;
    case InstructionId::Or:
        result = state.gpr(rs) | state.gpr(rb);
        write_result = logical_destination_ra = true;
        update_condition = record_form(word);
        break;
    case InstructionId::Xor:
        result = state.gpr(rs) ^ state.gpr(rb);
        write_result = logical_destination_ra = true;
        update_condition = record_form(word);
        break;

    case InstructionId::MoveFromCr:
        result = state.condition_register();
        write_result = true;
        break;
    case InstructionId::MoveToCrFields: {
        const auto mask = (word >> 12U) & 0xFFU;
        for (unsigned i = 0; i < 8U; ++i) {
            if ((mask & (1U << (7U - i))) != 0U) {
                const auto shift = (7U - i) * 4U;
                state.set_condition_field(i, static_cast<std::uint8_t>((state.gpr(rs) >> shift) & 0xFU));
            }
        }
        break;
    }
    case InstructionId::MoveFromSpr: {
        const auto spr = ((word >> 16U) & 0x1FU) | ((word >> 6U) & 0x3E0U);
        if (spr == 1U) result = state.xer();
        else if (spr == 8U) result = state.lr();
        else if (spr == 9U) result = state.ctr();
        else return ExecutionResult::UnsupportedEncoding;
        write_result = true;
        break;
    }
    case InstructionId::MoveToSpr: {
        const auto spr = ((word >> 16U) & 0x1FU) | ((word >> 6U) & 0x3E0U);
        const auto value = state.gpr(rs);
        if (spr == 1U) state.set_xer(value);
        else if (spr == 8U) state.set_lr(value);
        else if (spr == 9U) state.set_ctr(value);
        else return ExecutionResult::UnsupportedEncoding;
        break;
    }
    case InstructionId::Compare: {
        if ((word & (1U << 21U)) != 0U) return ExecutionResult::UnsupportedEncoding;
        const auto lhs = std::bit_cast<std::int32_t>(state.gpr(ra));
        const auto rhs = std::bit_cast<std::int32_t>(state.gpr(rb));
        const auto flags = static_cast<std::uint8_t>(
            (lhs < rhs ? 8U : 0U) |
            (lhs > rhs ? 4U : 0U) |
            (lhs == rhs ? 2U : 0U) |
            ((state.xer() >> 31U) & 1U));
        const auto field_index = (word >> 23U) & 7U;
        state.set_condition_field(field_index, flags);
        break;
    }
    default:
        return ExecutionResult::UnsupportedEncoding;
    }

    if (write_result) state.set_gpr(logical_destination_ra ? ra : rt, result);
    if (update_condition) update_cr0(state, result);
    state.set_pc(state.pc() + 4U);
    return ExecutionResult::Executed;
}
}
