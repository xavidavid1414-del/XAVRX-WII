#include "xavrx/system/wii_system.hpp"
#include <array>
namespace xavrx::system {
namespace {
constexpr std::uint32_t nand_base=0x0d010000U;
constexpr std::uint32_t usb_base=0x0d040000U;
constexpr std::uint32_t sd_base=0x0d070000U;
constexpr std::uint32_t wlan_base=0x0d080000U;
constexpr std::uint32_t drive_base=0x0d806000U;
}
bool ProtectedRegisterView::read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value)noexcept{
 if(!protection_.engine_access_allowed(cpu_,engine_))return false;
 return device_.read(offset,width,value);
}
bool ProtectedRegisterView::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value)noexcept{
 if(!protection_.engine_access_allowed(cpu_,engine_))return false;
 return device_.write(offset,width,value);
}
bool DriveResetView::read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value)noexcept{
 if(control_.di_io_reset_asserted()||control_.di_reset_b_asserted())return false;
 return device_.read(offset,width,value);
}
bool DriveResetView::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value)noexcept{
 if(control_.di_io_reset_asserted()||control_.di_reset_b_asserted())return false;
 return device_.write(offset,width,value);
}
WiiSystem::WiiSystem()
 :nand_broadway_(nand_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::FlashNand),
  nand_starlet_(nand_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::FlashNand),
  sd_broadway_(sd_host_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::Sd0),
  sd_starlet_(sd_host_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::Sd0),
  usb_broadway_(usb_host_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::Ehci),
  usb_starlet_(usb_host_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::Ehci),
  wlan_broadway_(wlan_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::Sd1),
  wlan_starlet_(wlan_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::Sd1),
  drive_broadway_(drive_,hollywood_.system_control()),drive_starlet_(drive_,hollywood_.system_control()){}
bool WiiSystem::initialize()noexcept{
 if(initialized_)return true;
 if(!hollywood_.map_into(memory_))return false;
 const std::array<DualDeviceMapping,5> mappings{{
  {nand_base,nand_.aperture_size(),&nand_broadway_,&nand_starlet_},
  {usb_base,usb_host_.aperture_size(),&usb_broadway_,&usb_starlet_},
  {sd_base,sd_host_.aperture_size(),&sd_broadway_,&sd_starlet_},
  {wlan_base,wlan_.aperture_size(),&wlan_broadway_,&wlan_starlet_},
  {drive_base,drive_.aperture_size(),&drive_broadway_,&drive_starlet_}
 }};
 if(!memory_.map_devices_dual(mappings.data(),mappings.size()))return false;
 initialized_=true;return true;
}
bool WiiSystem::submit_bluetooth_host_packet(HollywoodCpu cpu,std::span<const std::uint8_t> packet){
 return hollywood_.system_control().engine_access_allowed(cpu,HollywoodEngine::Usb1)&&bluetooth_hci_.submit_host_packet(packet);
}
bool WiiSystem::take_bluetooth_device_packet(HollywoodCpu cpu,std::vector<std::uint8_t>& packet){
 return hollywood_.system_control().engine_access_allowed(cpu,HollywoodEngine::Usb1)&&bluetooth_hci_.take_device_packet(packet);
}
bool WiiSystem::submit_bluetooth_device_packet(std::span<const std::uint8_t> packet){return bluetooth_hci_.submit_device_packet(packet);}
bool WiiSystem::take_bluetooth_host_packet(HollywoodCpu cpu,std::vector<std::uint8_t>& packet){
 return hollywood_.system_control().engine_access_allowed(cpu,HollywoodEngine::Usb1)&&bluetooth_hci_.take_host_packet(packet);
}
void WiiSystem::reset()noexcept{
 memory_.reset();hardware_.reset();hollywood_.reset();
 nand_.reset();sd_host_.reset();usb_host_.reset();drive_.reset();wlan_.reset();bluetooth_hci_.reset();
}
}
