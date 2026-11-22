#pragma once
#include "xavrx/system/wii_system.hpp"
#include "xavrx/core/broadway_cpu.hpp"
#include <memory>
#include <string_view>
namespace xavrx {
class Emulator final {
public:
 [[nodiscard]] static constexpr std::string_view version() noexcept{return "0.1.0";}
 [[nodiscard]] bool initialize() noexcept;
 [[nodiscard]] bool initialized() const noexcept;
 [[nodiscard]] system::WiiSystem* system() noexcept{return system_.get();}
 [[nodiscard]] const system::WiiSystem* system() const noexcept{return system_.get();}
 [[nodiscard]] BroadwayCpu& broadway_cpu() noexcept{return broadway_cpu_;}
 [[nodiscard]] CpuStepResult step_broadway() noexcept;
 void reset() noexcept;
private:
 std::unique_ptr<system::WiiSystem> system_{};
 BroadwayCpu broadway_cpu_{};
};
}
