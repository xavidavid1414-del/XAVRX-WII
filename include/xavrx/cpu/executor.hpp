#pragma once
#include "xavrx/cpu/cpu_state.hpp"
#include <cstdint>
namespace xavrx::cpu {
enum class ExecutionResult : std::uint8_t { Executed, InvalidInstruction, UnsupportedEncoding };
class IntegerExecutor final {
public:
 // Executes one supported 32-bit integer instruction and advances PC by four.
 // Unsupported encodings leave architectural state unchanged.
 [[nodiscard]] static ExecutionResult execute(std::uint32_t word,CpuState& state) noexcept;
};
}
