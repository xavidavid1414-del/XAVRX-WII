#pragma once
#include "xavrx/cpu/instruction.hpp"
#include <cstdint>
namespace xavrx::cpu {
class InstructionDecoder final {
public:
 [[nodiscard]] static DecodedInstruction decode(std::uint32_t word) noexcept;
};
}