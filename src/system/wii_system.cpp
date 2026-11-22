#include "xavrx/system/wii_system.hpp"
#include <array>
#include <cmath>
namespace xavrx::system {
namespace {
constexpr std::uint32_t nand_base=0x0d010000U;
constexpr std::uint32_t usb_base=0x0d040000U;
constexpr std::uint32_t sd_base=0x0d070000U;
constexpr std::uint32_t wlan_base=0x0d080000U;
constexpr std::uint32_t drive_base=0x0d806000U;
constexpr std::uint32_t pi_base=0x0c003000U;
constexpr std::uint32_t dsp_base=0x0c005000U;
constexpr std::uint32_t gx_fifo_base=0x0c008000U;
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
GameInputState WiiSystem::route_controller_inputs()const noexcept{
 GameInputState combined{};
 const auto merge=[this,&combined](const ControllerInputFrame& frame){
  const auto routed=input_router_.route(frame);
  for(std::size_t i=0;i<combined.buttons.size();++i){
   combined.buttons[i]=combined.buttons[i]||routed.buttons[i];
   if(std::abs(routed.axes[i])>std::abs(combined.axes[i]))combined.axes[i]=routed.axes[i];
  }
 };
 merge(make_input_frame(wii_remote_.state()));
 merge(make_input_frame(nunchuk_.state()));
 merge(make_input_frame(classic_controller_.state()));
 merge(make_input_frame(classic_controller_pro_.state()));
 merge(make_input_frame(balance_board_.state()));
 return combined;
}
void WiiSystem::advance_broadway_cycles(std::uint64_t cycles)noexcept{
 hardware_.advance(cycles);
 reset_controller_.set_register(hollywood_.system_control().reset_register());
 if(reset_controller_.asserted(HardwareResetLine::DSP))dsp_.reset();
 if(reset_controller_.asserted(HardwareResetLine::GFX)||reset_controller_.asserted(HardwareResetLine::GFXTCPE))gpu_.reset();
 ClockDomainScheduler::Frequencies targets{};
 for(std::size_t i=0;i<ClockDomainScheduler::domain_count;++i){
  const auto domain=static_cast<ClockDomain>(i);
  targets[i]=hardware_.clock().frequency(domain);
 }
 targets[static_cast<std::size_t>(ClockDomain::Starlet)]=hollywood_.system_control().hollywood_clock_hz();
 const auto produced=clock_scheduler_.advance(cycles,hardware_.clock().frequency(ClockDomain::Cpu),targets);
 hollywood_.advance_timer(produced[static_cast<std::size_t>(ClockDomain::Starlet)]);
 if(reset_controller_.released(HardwareResetLine::DSP))dsp_.advance(produced[static_cast<std::size_t>(ClockDomain::Dsp)]);
}
bool WiiSystem::initialize()noexcept{
 if(initialized_)return true;
 if(!hollywood_.map_into(memory_))return false;
 const std::array<DualDeviceMapping,8> mappings{{
  {nand_base,nand_.aperture_size(),&nand_broadway_,&nand_starlet_},
  {usb_base,usb_host_.aperture_size(),&usb_broadway_,&usb_starlet_},
  {sd_base,sd_host_.aperture_size(),&sd_broadway_,&sd_starlet_},
  {wlan_base,wlan_.aperture_size(),&wlan_broadway_,&wlan_starlet_},
  {drive_base,drive_.aperture_size(),&drive_broadway_,&drive_starlet_},
  {pi_base,0x100U,&processor_interrupts_,&pi_denied_},
  {dsp_base,0x200U,&dsp_mailbox_,&dsp_denied_},
  {gx_fifo_base,4U,&gx_fifo_,&gx_denied_}
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
 nand_.reset();sd_host_.reset();usb_host_.reset();drive_.reset();wlan_.reset();bluetooth_hci_.reset();clock_scheduler_.reset();
 wii_remote_.reset();nunchuk_.reset();classic_controller_.reset();classic_controller_pro_.reset();balance_board_.reset();
 reset_controller_.reset();dsp_.reset();gpu_.reset();dsp_mailbox_.reset();processor_interrupts_.reset();
}
}
