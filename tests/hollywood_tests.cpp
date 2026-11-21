#include "xavrx/system/hollywood.hpp"
#include "xavrx/memory/memory.hpp"
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){using namespace xavrx::system;HollywoodHardware hw;std::uint64_t value=0;
 hw.interrupts().raise(4,IrqRoute::Broadway);check(hw.interrupts().flags(HollywoodCpu::Broadway)==(1U<<4U)&&hw.interrupts().flags(HollywoodCpu::Starlet)==0,"IRQ source routing");
 check(hw.write_as(HollywoodCpu::Broadway,0x34,4,1U<<4U),"PPC IRQ mask write");check(hw.interrupts().asserted(HollywoodCpu::Broadway),"PPC IRQ asserted by mask");check(hw.write_as(HollywoodCpu::Broadway,0x30,4,1U<<4U),"PPC IRQ W1C");check(!hw.interrupts().asserted(HollywoodCpu::Broadway),"PPC IRQ cleared");
 check(hw.write_as(HollywoodCpu::Broadway,0,4,0x12345678U),"IPC PPC message write");check(hw.read_as(HollywoodCpu::Starlet,0,4,value)&&value==0x12345678U,"IPC message shared");
 check(!hw.read_as(HollywoodCpu::Broadway,12,4,value),"ARM control is Starlet-only");
 check(hw.write_as(HollywoodCpu::Broadway,4,4,1U),"Broadway sets X1");check(hw.read_as(HollywoodCpu::Starlet,12,4,value)&&(value&4U)!=0U,"Starlet observes X1");check(hw.write_as(HollywoodCpu::Starlet,12,4,4U),"Starlet clears X1");check(hw.read_as(HollywoodCpu::Broadway,4,4,value)&&(value&1U)==0U,"X1 clear reflected to Broadway");
 check(hw.write_as(HollywoodCpu::Starlet,12,4,1U|(1U<<4U)),"Starlet sets Y1 and enables IRQ");check(hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<30U),"IPC Y1 raises IRQ 30");
 check(hw.write_as(HollywoodCpu::Starlet,0xe0,4,1U),"Starlet GPIO output write");check(hw.write_as(HollywoodCpu::Starlet,0xe4,4,1U),"Starlet GPIO direction write");check(hw.read_as(HollywoodCpu::Starlet,0xe4,4,value)&&value==1U,"Starlet GPIO direction read");
 check(hw.write_as(HollywoodCpu::Starlet,0xfc,4,1U),"GPIO owner selects Broadway pin");check(hw.write_as(HollywoodCpu::Broadway,0xc0,4,1U),"Broadway controls owned GPIO output");check(hw.read_as(HollywoodCpu::Broadway,0xc0,4,value)&&value==1U,"Broadway GPIO output retained");
 check(hw.write_as(HollywoodCpu::Broadway,0xcc,4,1U),"GPIO interrupt level set");check(hw.write_as(HollywoodCpu::Broadway,0xd4,4,1U),"GPIO interrupt mask set");hw.gpio().set_inputs(1U);check((hw.interrupts().flags(HollywoodCpu::Broadway)&(1U<<10U))!=0U,"Broadway GPIO triggers IRQ 10");
 check(hw.write_as(HollywoodCpu::Broadway,0xd0,4,1U),"GPIO interrupt flag W1C");
 check(!hw.read_as(HollywoodCpu::Broadway,0xe0,4,value),"Broadway cannot access Starlet GPIO bank");
 check(!hw.read_as(HollywoodCpu::Starlet,0xc0,4,value),"Starlet cannot access Broadway GPIO bank");
 xavrx::memory::Memory bus(0x80000000U,1024U);check(bus.map_device(HollywoodHardware::base,HollywoodHardware::window_size,hw),"map Hollywood MMIO");check(bus.map_device(HollywoodHardware::mirror,HollywoodHardware::window_size,hw),"map Hollywood MMIO mirror");check(bus.write32(HollywoodHardware::base+4U,0xabcdef01U)==xavrx::memory::AccessResult::Success,"MMIO PPCCTRL bus write");check(bus.read32(HollywoodHardware::mirror, value)==xavrx::memory::AccessResult::Success,"MMIO mirror bus read");check(value==0U,"MMIO mirror reads same hardware register space");
 hw.reset();check(hw.interrupts().flags(HollywoodCpu::Broadway)==0U,"Hollywood reset clears IRQ state");
 if(failures){std::cerr<<failures<<" Hollywood test(s) failed\n";return 1;}std::cout<<"All Hollywood hardware tests passed\n";return 0;}
