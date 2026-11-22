#pragma once
#include "xavrx/cpu/cpu_state.hpp"
#include <cstdint>
namespace xavrx::system { class WiiSystem; }
namespace xavrx::core {
enum class CpuStepResult : std::uint8_t { Executed, InterruptTaken, InvalidInstruction, UnsupportedInstruction, BusFault, NotInitialized };
class BroadwayCpu final {
public:
 [[nodiscard]] CpuStepResult step(system::WiiSystem& system) noexcept;
 [[nodiscard]] CpuState& state() noexcept{return state_;}
 [[nodiscard]] const CpuState& state() const noexcept{return state_;}
 void reset() noexcept{state_.reset();}
private:
 cpu::CpuState state_{};
};
}
