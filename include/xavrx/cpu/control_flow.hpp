#pragma once
#include "xavrx/cpu/cpu_state.hpp"
#include <cstdint>
namespace xavrx::cpu {
enum class ControlFlowResult : std::uint8_t { Executed, InvalidInstruction, UnsupportedEncoding, InterruptMasked };
enum class ExceptionKind : std::uint8_t { SystemCall, Program, ExternalInterrupt };
class ControlFlowExecutor final {
public:
 [[nodiscard]] static ControlFlowResult execute(std::uint32_t word,CpuState& state) noexcept;
 [[nodiscard]] static ControlFlowResult raise_exception(ExceptionKind kind,CpuState& state) noexcept;
 [[nodiscard]] static ControlFlowResult return_from_interrupt(CpuState& state) noexcept;
};
}
