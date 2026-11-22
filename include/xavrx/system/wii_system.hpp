#pragma once
#include "xavrx/system/hardware.hpp"
#include "xavrx/system/reset_controller.hpp"
#include "xavrx/system/dsp_gpu.hpp"
#include "xavrx/system/broadway_devices.hpp"
#include "xavrx/system/controllers.hpp"
#include "xavrx/system/hollywood.hpp"
#include "xavrx/system/peripherals.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <cstdint>
#include <span>
#include <vector>
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
class DriveResetView final : public memory::MemoryMappedDevice {
public:
 DriveResetView(DriveInterface& device,HollywoodSystemControl& control) noexcept:device_(device),control_(control){}
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
private:
 DriveInterface& device_;
 HollywoodSystemControl& control_;
};
class DeniedDeviceView final : public memory::MemoryMappedDevice {
public:
 [[nodiscard]] bool read(std::uint32_t,std::uint8_t,std::uint64_t&) noexcept override{return false;}
 [[nodiscard]] bool write(std::uint32_t,std::uint8_t,std::uint64_t) noexcept override{return false;}
};
class WiiSystem final {
public:
 WiiSystem();
 [[nodiscard]] bool initialize() noexcept;
 [[nodiscard]] bool initialized() const noexcept{return initialized_;}
 [[nodiscard]] WiiMemory& memory() noexcept{return memory_;}
 [[nodiscard]] SystemHardware& hardware() noexcept{return hardware_;}
 [[nodiscard]] HollywoodHardware& hollywood() noexcept{return hollywood_;}
 [[nodiscard]] NandInterface& nand() noexcept{return nand_;}
 [[nodiscard]] SdHostController& sd_host() noexcept{return sd_host_;}
 [[nodiscard]] UsbHostController& usb_host() noexcept{return usb_host_;}
 [[nodiscard]] DriveInterface& drive_interface() noexcept{return drive_;}
 [[nodiscard]] WirelessLanController& wlan() noexcept{return wlan_;}
 [[nodiscard]] WiiRemoteController& wii_remote() noexcept{return wii_remote_;}
 [[nodiscard]] NunchukController& nunchuk() noexcept{return nunchuk_;}
 [[nodiscard]] ClassicController& classic_controller() noexcept{return classic_controller_;}
 [[nodiscard]] HardwareResetController& reset_controller() noexcept{return reset_controller_;}
 [[nodiscard]] DspDevice& dsp() noexcept{return dsp_;}
 [[nodiscard]] GpuCommandProcessor& gpu() noexcept{return gpu_;}
 [[nodiscard]] DspMailboxInterface& dsp_mailbox() noexcept{return dsp_mailbox_;}
 [[nodiscard]] ProcessorInterruptInterface& processor_interrupts() noexcept{return processor_interrupts_;}
 [[nodiscard]] bool submit_bluetooth_host_packet(HollywoodCpu cpu,std::span<const std::uint8_t> packet);
 [[nodiscard]] bool take_bluetooth_device_packet(HollywoodCpu cpu,std::vector<std::uint8_t>& packet);
 [[nodiscard]] bool submit_bluetooth_device_packet(std::span<const std::uint8_t> packet);
 [[nodiscard]] bool take_bluetooth_host_packet(HollywoodCpu cpu,std::vector<std::uint8_t>& packet);
 void advance_broadway_cycles(std::uint64_t cycles) noexcept;
 [[nodiscard]] std::uint64_t scheduled_cycles(ClockDomain domain) const noexcept{return clock_scheduler_.total_cycles(domain);}
 void advance_starlet_ticks(std::uint64_t ticks) noexcept{hollywood_.advance_timer(ticks);}
 void reset() noexcept;
private:
 WiiMemory memory_;
 SystemHardware hardware_;
 HollywoodHardware hollywood_;
 NandInterface nand_;
 SdHostController sd_host_;
 UsbHostController usb_host_;
 DriveInterface drive_;
 WirelessLanController wlan_;
 BluetoothHciTransport bluetooth_hci_;
 WiiRemoteController wii_remote_{};
 NunchukController nunchuk_{};
 ClassicController classic_controller_{};
 ClockDomainScheduler clock_scheduler_;
 HardwareResetController reset_controller_{};
 DspDevice dsp_{};
 GpuCommandProcessor gpu_{};
 DspMailboxInterface dsp_mailbox_{};
 ProcessorInterruptInterface processor_interrupts_{};
 GxWriteGatherPort gx_fifo_{gpu_};
 DeniedDeviceView dsp_denied_{},pi_denied_{},gx_denied_{};
 ProtectedRegisterView nand_broadway_,nand_starlet_;
 ProtectedRegisterView sd_broadway_,sd_starlet_;
 ProtectedRegisterView usb_broadway_,usb_starlet_;
 ProtectedRegisterView wlan_broadway_,wlan_starlet_;
 DriveResetView drive_broadway_,drive_starlet_;
 bool initialized_{false};
};
}
