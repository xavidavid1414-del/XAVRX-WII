#pragma once
#include "xavrx/cpu/cpu_state.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <cstdint>
namespace xavrx::cpu {
enum class MemoryExecutionResult : std::uint8_t { Executed, UnsupportedEncoding, InvalidInstruction, BusFault };
class LoadStoreExecutor final {
public:
 [[nodiscard]] static MemoryExecutionResult execute(std::uint32_t word,CpuState& state,system::WiiMemory& memory) noexcept;
};
}
