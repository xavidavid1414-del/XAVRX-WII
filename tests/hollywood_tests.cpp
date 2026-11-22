#include "xavrx/system/hollywood.hpp"
#include "xavrx/memory/memory.hpp"
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;
 HollywoodHardware hw;std::uint64_t value=0;std::uint32_t word=0;
 hw.interrupts().raise(4,IrqRoute::Broadway);
 check(hw.interrupts().flags(HollywoodCpu::Broadway)==(1U<<4U)&&hw.interrupts().flags(HollywoodCpu::Starlet)==0,"IRQ source routing");
 check(hw.write_as(HollywoodCpu::Broadway,0x34,4,1U<<4U),"PPC IRQ mask write");
 check(hw.interrupts().asserted(HollywoodCpu::Broadway),"PPC IRQ asserted by mask");
 check(hw.write_as(HollywoodCpu::Broadway,0x30,4,1U<<4U),"PPC IRQ W1C");
 check(!hw.interrupts().asserted(HollywoodCpu::Broadway),"PPC IRQ cleared");

 check(hw.write_as(HollywoodCpu::Broadway,0,4,0x12345678U),"IPC PPC message write");
 check(hw.read_as(HollywoodCpu::Starlet,0,4,value)&&value==0x12345678U,"IPC message shared");
 check(!hw.read_as(HollywoodCpu::Broadway,12,4,value),"ARM control is Starlet-only");
 check(!hw.write_as(HollywoodCpu::Starlet,4,4,1U),"Starlet cannot write PPC control");
 check(hw.write_as(HollywoodCpu::Broadway,4,4,1U),"Broadway sets X1");
 check(hw.read_as(HollywoodCpu::Starlet,12,4,value)&&(value&4U)!=0U,"Starlet observes X1");
 check(hw.write_as(HollywoodCpu::Starlet,12,4,1U<<4U),"Starlet enables X1 IRQ");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&(1U<<31U))!=0U,"X1 IRQ routes to Starlet");
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<31U))==0U,"X1 IRQ not routed to Broadway");
 check(hw.write_as(HollywoodCpu::Starlet,12,4,4U),"Starlet clears X1");
 check(hw.read_as(HollywoodCpu::Broadway,4,4,value)&&(value&1U)==0U,"X1 clear reflected to Broadway");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&(1U<<31U))==0U,"clearing X1 deasserts IPC IRQ");

 check(hw.write_as(HollywoodCpu::Starlet,12,4,1U),"Starlet sets Y1");
 check(hw.write_as(HollywoodCpu::Broadway,4,4,1U<<4U),"Broadway enables Y1 IRQ");
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<30U))!=0U,"Y1 IRQ routes to Broadway");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&(1U<<30U))==0U,"Y1 IRQ not routed to Starlet");
 check(hw.write_as(HollywoodCpu::Broadway,4,4,(1U<<4U)|(1U<<2U)),"Broadway clears Y1");
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<30U))==0U,"clearing Y1 deasserts IPC IRQ");

 check(hw.read_as(HollywoodCpu::Starlet,HollywoodSystemControl::clocks_register,4,value)&&value==0U,"Starlet reads Hollywood clock control");
 check(hw.system_control().hollywood_clock_hz()==243000000ULL,"Hollywood starts in Wii clock mode");
 check(hw.write_as(HollywoodCpu::Starlet,HollywoodSystemControl::clocks_register,4,3U),"Starlet writes FX and SPEED clock controls");
 check(hw.read_as(HollywoodCpu::Starlet,HollywoodSystemControl::clocks_register,4,value)&&value==3U,"Hollywood clock control readback");
 check(hw.system_control().fx_enabled()&&hw.system_control().hollywood_clock_hz()==162000000ULL,"clock control exposes documented mode");
 check(!hw.write_as(HollywoodCpu::Broadway,HollywoodSystemControl::clocks_register,4,0U),"Broadway cannot access Starlet clock control");
 check(!hw.read_as(HollywoodCpu::Broadway,HollywoodSystemControl::resets_register,4,value),"Broadway cannot access Starlet reset control");
 check(hw.write_as(HollywoodCpu::Starlet,0x40,4,1U<<7U),"Starlet writes FIQ mask");
 hw.interrupts().raise(7,IrqRoute::Starlet);
 check(hw.interrupts().fiq_asserted(),"Starlet FIQ asserted by masked source");
 check(hw.read_as(HollywoodCpu::Starlet,0x40,4,value)&&value==(1U<<7U),"Starlet FIQ mask readback");
 check(!hw.read_as(HollywoodCpu::Broadway,0x40,4,value),"Broadway cannot access Starlet FIQ mask");
 check(hw.write_as(HollywoodCpu::Starlet,HollywoodSystemControl::resets_register,4,0xffffffffU),"Starlet writes reset control register");
 check(hw.read_as(HollywoodCpu::Starlet,HollywoodSystemControl::resets_register,4,value)&&value==HollywoodSystemControl::resets_writable_mask,"reserved reset bit is masked");
 check(hw.system_control().reset_released(1U),"reset line state can be inspected");
 check(hw.write_as(HollywoodCpu::Starlet,HollywoodTimer::alarm_register,4,3U),"Starlet programs timer alarm");
 check(!hw.write_as(HollywoodCpu::Broadway,HollywoodTimer::alarm_register,4,9U),"Broadway cannot program Starlet alarm");
 check(hw.read_as(HollywoodCpu::Starlet,HollywoodTimer::timer_register,4,value)&&value==0U,"Starlet reads timer counter");
 check(!hw.read_as(HollywoodCpu::Broadway,HollywoodTimer::timer_register,4,value),"Broadway cannot read Starlet timer");
 check(hw.write_as(HollywoodCpu::Starlet,0x3c,4,1U),"Starlet enables timer IRQ");
 hw.advance_timer(2U);check(hw.timer().counter()==2U&&hw.timer().armed(),"timer counts before alarm deadline");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&1U)==0U,"timer IRQ not early");
 hw.advance_timer(1U);check(hw.timer().counter()==3U&&!hw.timer().armed(),"timer reaches alarm deadline");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&1U)!=0U,"timer alarm raises Starlet IRQ zero");
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&1U)==0U,"timer IRQ is Starlet-only");
 check(hw.interrupts().asserted(HollywoodCpu::Starlet),"timer IRQ mask asserts");
 check(hw.write_as(HollywoodCpu::Starlet,0x38,4,1U),"Starlet acknowledges timer IRQ");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&1U)==0U,"timer IRQ acknowledged");

 check(hw.write_as(HollywoodCpu::Starlet,0xec,4,0x00ffffffU),"Initialize GPIO pins for active-high interrupts");
 check(hw.write_as(HollywoodCpu::Starlet,0xfc,4,1U),"Starlet assigns GPIO pin zero to Broadway");
 check(hw.write_as(HollywoodCpu::Starlet,0xe0,4,1U),"Starlet cannot overwrite Broadway-owned output");
 check(hw.write_as(HollywoodCpu::Broadway,0xc0,4,1U),"Broadway writes owned GPIO output");
 check(hw.read_as(HollywoodCpu::Starlet,0xe0,4,value)&&value==1U,"GPIO output storage is shared");
 check(hw.read_as(HollywoodCpu::Starlet,0xc0,4,value)&&value==1U,"Starlet can inspect Broadway GPIO view");
 check(hw.write_as(HollywoodCpu::Broadway,0xcc,4,1U),"Broadway configures owned GPIO interrupt level");
 check(hw.write_as(HollywoodCpu::Broadway,0xd4,4,1U),"Broadway configures owned GPIO interrupt mask");
 hw.gpio().set_inputs(1U);
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<10U))!=0U,"owned GPIO pin raises IRQ 10");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&(1U<<10U))==0U,"GPIOB IRQ is Broadway-only");
 check(hw.write_as(HollywoodCpu::Broadway,0xd0,4,1U),"GPIO W1C write accepted");
 check((hw.gpio().read(HollywoodCpu::Broadway,0xd0,word)&&word==1U),"GPIO flag cannot clear while pin is active");
 hw.gpio().set_inputs(0U);
 check(hw.write_as(HollywoodCpu::Broadway,0xd0,4,1U),"GPIO flag clears after pin becomes idle");
 check(hw.gpio().read(HollywoodCpu::Broadway,0xd0,word)&&word==0U,"GPIO W1C clears idle flag");
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<10U))==0U,"cleared GPIO flag deasserts IRQ 10");

 check(hw.write_as(HollywoodCpu::Starlet,0xec,4,0x00ffffffU),"Starlet preserves active-high levels across full GPIO bank");
 check(hw.write_as(HollywoodCpu::Starlet,0xf4,4,2U),"Starlet configures unowned GPIO interrupt mask");
 hw.gpio().set_inputs(2U);
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&(1U<<11U))!=0U,"Starlet-owned GPIO pin raises IRQ 11");
 check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<11U))==0U,"GPIO IRQ 11 is Starlet-only");
 check(hw.write_as(HollywoodCpu::Starlet,0xf0,4,2U),"Starlet clears idle GPIO flag after input returns");
 hw.gpio().set_inputs(0U);
 check(hw.write_as(HollywoodCpu::Starlet,0xf0,4,2U),"Starlet W1C clears GPIO flag");
 check(hw.gpio().read(HollywoodCpu::Starlet,0xf0,word)&&word==0U,"shared GPIO flag bank cleared");

 check(!hw.read_as(HollywoodCpu::Broadway,0xdc,4,value),"Broadway cannot access GPIO enable");
 check(!hw.read_as(HollywoodCpu::Broadway,0xe0,4,value),"Broadway cannot access full Starlet GPIO bank");
 check(!hw.write_as(HollywoodCpu::Broadway,0xfc,4,0U),"Broadway cannot change GPIO owner");

 xavrx::memory::Memory ppc_bus(0x80000000U,1024U),starlet_bus(0x80000000U,1024U);
 check(hw.map_into(ppc_bus,starlet_bus),"map CPU-specific Hollywood MMIO views");
 check(ppc_bus.write32(HollywoodHardware::base+4U,0xabcdef01U)==xavrx::memory::AccessResult::Success,"Broadway MMIO PPCCTRL write");
 check(starlet_bus.read32(HollywoodHardware::mirror,word)==xavrx::memory::AccessResult::Success,"Starlet MMIO mirror read");
 check(word==0x12345678U,"Starlet MMIO sees shared PPC message");
 check(starlet_bus.write32(HollywoodHardware::mirror+12U,1U)==xavrx::memory::AccessResult::Success,"Starlet MMIO ARMCTRL write");
 check(ppc_bus.read32(HollywoodHardware::base+12U,word)==xavrx::memory::AccessResult::DeviceError,"Broadway MMIO cannot access ARMCTRL");
 check(ppc_bus.read32(HollywoodHardware::mirror,word)==xavrx::memory::AccessResult::Unmapped,"Broadway bus has no Starlet private mirror");

 check(hw.read_as(HollywoodCpu::Broadway,HollywoodSystemControl::ahb_prot_register,4,value)&&value==0U,"Broadway can inspect AHB protection state");
 check(!hw.write_as(HollywoodCpu::Broadway,HollywoodSystemControl::ahb_prot_register,4,1U<<31U),"Broadway cannot grant itself privileged access");
 check(hw.write_as(HollywoodCpu::Starlet,HollywoodSystemControl::ahb_prot_register,4,1U<<31U),"Starlet grants PPCKERN access");
 check(hw.read_as(HollywoodCpu::Broadway,12,4,value),"PPCKERN grants Broadway ARMCTRL visibility");
 check(hw.read_as(HollywoodCpu::Broadway,0x38,4,value),"PPCKERN grants Broadway Starlet IRQ register visibility");
 check(hw.write_as(HollywoodCpu::Broadway,0x40,4,1U<<6U),"PPCKERN grants Broadway FIQ mask access");
 check(hw.write_as(HollywoodCpu::Broadway,0xe0,4,2U),"PPCKERN grants Broadway full GPIO bank access");
 check(hw.read_as(HollywoodCpu::Broadway,0xe0,4,value)&&(value&2U)!=0U,"privileged GPIO write is shared");
 hw.reset();
 check(hw.interrupts().flags(HollywoodCpu::Broadway)==0U&&hw.interrupts().flags(HollywoodCpu::Starlet)==0U,"Hollywood reset clears IRQ state");
 check(hw.timer().counter()==0U&&!hw.timer().armed(),"Hollywood reset clears timer");
 check(hw.system_control().hollywood_clock_hz()==243000000ULL&&!hw.system_control().fx_enabled()&&hw.system_control().reset_register()==0U&&hw.system_control().ahb_protection()==0U,"Hollywood reset clears clock/reset/AHB control state");
 check(hw.gpio().read(HollywoodCpu::Starlet,0xf0,word)&&word==0U,"Hollywood reset leaves GPIO flags clear");
 hw.advance_timer(0xfffffffeULL);
 check(hw.write_as(HollywoodCpu::Starlet,HollywoodTimer::alarm_register,4,1U),"Program alarm across timer wraparound");
 hw.advance_timer(2U);
 check(hw.timer().counter()==0U&&hw.timer().armed(),"Timer wraps without early alarm");
 hw.advance_timer(1U);
 check(hw.timer().counter()==1U&&!hw.timer().armed(),"Alarm fires after wrapped deadline");
 check((hw.interrupts().flags(HollywoodCpu::Starlet)&1U)!=0U,"Wrapped alarm raises timer IRQ");
 if(failures){std::cerr<<failures<<" Hollywood test(s) failed\n";return 1;}
 std::cout<<"All Hollywood hardware tests passed\n";return 0;
}
