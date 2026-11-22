#include "xavrx/system/wii_system.hpp"
#include <array>
namespace xavrx::system {
namespace {
constexpr std::uint32_t nand_base=0x0d010000U;
constexpr std::uint32_t sd_base=0x0d070000U;
constexpr std::uint32_t usb_base=0x0d040000U;
}
bool ProtectedRegisterView::read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value)noexcept{
 if(!protection_.engine_access_allowed(cpu_,engine_))return false;
 return device_.read(offset,width,value);
}
bool ProtectedRegisterView::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value)noexcept{
 if(!protection_.engine_access_allowed(cpu_,engine_))return false;
 return device_.write(offset,width,value);
}
WiiSystem::WiiSystem()noexcept
 :nand_broadway_(nand_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::FlashNand),
  nand_starlet_(nand_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::FlashNand),
  sd_broadway_(sd_host_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::Sd0),
  sd_starlet_(sd_host_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::Sd0),
  usb_broadway_(usb_host_,hollywood_.system_control(),HollywoodCpu::Broadway,HollywoodEngine::Ehci),
  usb_starlet_(usb_host_,hollywood_.system_control(),HollywoodCpu::Starlet,HollywoodEngine::Ehci){}
bool WiiSystem::initialize()noexcept{
 if(initialized_)return true;
 if(!hollywood_.map_into(memory_))return false;
 const std::array<DualDeviceMapping,3> mappings{{
  {nand_base,nand_.aperture_size(),&nand_broadway_,&nand_starlet_},
  {sd_base,sd_host_.aperture_size(),&sd_broadway_,&sd_starlet_},
  {usb_base,usb_host_.aperture_size(),&usb_broadway_,&usb_starlet_}
 }};
 if(!memory_.map_devices_dual(mappings.data(),mappings.size()))return false;
 initialized_=true;return true;
}
void WiiSystem::reset()noexcept{
 memory_.reset();hardware_.reset();hollywood_.reset();
 nand_.reset();sd_host_.reset();usb_host_.reset();
}
}
