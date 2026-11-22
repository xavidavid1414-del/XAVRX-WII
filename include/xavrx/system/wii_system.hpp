#pragma once
#include "xavrx/system/hardware.hpp"
#include "xavrx/system/hollywood.hpp"
#include "xavrx/system/peripherals.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <cstdint>
namespace xavrx::system {
class ProtectedRegisterView final : public memory::MemoryMappedDevice {
public:
 ProtectedRegisterView(RegisterWindow& device,HollywoodSystemControl& protection,HollywoodCpu cpu,HollywoodEngine engine) noexcept
  :device_(device),protection_(protection),cpu_(cpu),engine_(engine){}
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
private:
 RegisterWindow& device_;
 HollywoodSystemControl& protection_;
 HollywoodCpu cpu_;
 HollywoodEngine engine_;
};
class WiiSystem final {
public:
 WiiSystem() noexcept;
 [[nodiscard]] bool initialize() noexcept;
 [[nodiscard]] bool initialized() const noexcept{return initialized_;}
 [[nodiscard]] WiiMemory& memory() noexcept{return memory_;}
 [[nodiscard]] SystemHardware& hardware() noexcept{return hardware_;}
 [[nodiscard]] HollywoodHardware& hollywood() noexcept{return hollywood_;}
 [[nodiscard]] NandInterface& nand() noexcept{return nand_;}
 [[nodiscard]] SdHostController& sd_host() noexcept{return sd_host_;}
 [[nodiscard]] UsbHostController& usb_host() noexcept{return usb_host_;}
 void advance_broadway_cycles(std::uint64_t cycles) noexcept{hardware_.advance(cycles);}
 void advance_starlet_ticks(std::uint64_t ticks) noexcept{hollywood_.advance_timer(ticks);}
 void reset() noexcept;
private:
 WiiMemory memory_;
 SystemHardware hardware_;
 HollywoodHardware hollywood_;
 NandInterface nand_;
 SdHostController sd_host_;
 UsbHostController usb_host_;
 ProtectedRegisterView nand_broadway_,nand_starlet_;
 ProtectedRegisterView sd_broadway_,sd_starlet_;
 ProtectedRegisterView usb_broadway_,usb_starlet_;
 bool initialized_{false};
};
}
