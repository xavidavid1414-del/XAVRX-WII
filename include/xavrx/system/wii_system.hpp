#pragma once
#include "xavrx/memory/memory.hpp"
#include "xavrx/system/hardware.hpp"
#include "xavrx/system/hollywood.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <cstdint>
namespace xavrx::system {
class WiiSystem final {
public:
 WiiSystem();
 [[nodiscard]] bool initialize() noexcept;
 [[nodiscard]] bool initialized() const noexcept{return initialized_;}
 [[nodiscard]] WiiMemory& memory() noexcept{return memory_;}
 [[nodiscard]] SystemHardware& hardware() noexcept{return hardware_;}
 [[nodiscard]] HollywoodHardware& hollywood() noexcept{return hollywood_;}
 [[nodiscard]] memory::Memory& broadway_io_bus() noexcept{return broadway_io_bus_;}
 [[nodiscard]] memory::Memory& starlet_io_bus() noexcept{return starlet_io_bus_;}
 void advance_broadway_cycles(std::uint64_t cycles) noexcept{hardware_.advance(cycles);}
 void advance_starlet_ticks(std::uint64_t ticks) noexcept{hollywood_.advance_timer(ticks);}
 void reset() noexcept;
private:
 WiiMemory memory_;
 SystemHardware hardware_;
 HollywoodHardware hollywood_;
 memory::Memory broadway_io_bus_;
 memory::Memory starlet_io_bus_;
 bool initialized_{false};
};
}
