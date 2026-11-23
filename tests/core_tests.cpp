#include "xavrx/core/emulator.hpp"
#include "xavrx/system/hollywood.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <cstdint>
#include <iostream>
#include <string_view>
namespace {int failures=0;void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}}}
int main(){
 xavrx::Emulator emulator;
 check(xavrx::Emulator::version()==std::string_view{"1.0.0-alpha.1"},"version smoke test");
 check(!emulator.initialized(),"fresh emulator is not initialized");
 check(emulator.initialize(),"initialize composed Wii hardware");
 check(emulator.initialized(),"emulator reports initialized after hardware setup");
 check(emulator.initialize(),"initialization is idempotent");
 auto* system=emulator.system();
 check(system!=nullptr,"system object is available");
 if(system){
  check(system->initialized(),"Wii system reports initialized");
  check(system->memory().mem1_capacity()==xavrx::system::WiiMemory::mem1_size,"MEM1 is present");
  check(system->memory().mem2_capacity()==xavrx::system::WiiMemory::mem2_size,"MEM2 is present");
  check(system->memory().write32(xavrx::system::WiiMemory::mem1_cached,0x12345678U)==xavrx::memory::AccessResult::Success,"write through cached MEM1 alias");
  std::uint32_t word=0;std::uint64_t value=0;
  check(system->memory().read32(xavrx::system::WiiMemory::mem1_physical,word)==xavrx::memory::AccessResult::Success&&word==0x12345678U,"MEM1 physical alias shares backing");
  check(system->memory().write_as(xavrx::system::WiiMemoryCpu::Broadway,xavrx::system::HollywoodHardware::base,4,0x87654321U)==xavrx::memory::AccessResult::Success,"Broadway reaches Hollywood MMIO through Wii bus");
  check(system->memory().read_as(xavrx::system::WiiMemoryCpu::Starlet,xavrx::system::HollywoodHardware::mirror,4,value)==xavrx::memory::AccessResult::Success&&value==0x87654321U,"Starlet sees shared Hollywood message register");
  check(system->memory().write_as(xavrx::system::WiiMemoryCpu::Broadway,xavrx::system::HollywoodHardware::mirror+12U,4,1U)==xavrx::memory::AccessResult::DeviceError,"Broadway cannot access Starlet-private ARMCTRL through mirror");
  check(system->memory().read_as(xavrx::system::WiiMemoryCpu::Starlet,xavrx::system::HollywoodHardware::base+12U,4,value)==xavrx::memory::AccessResult::Success,"Starlet has full Hollywood register access through shared window");
  check(system->hollywood().write_as(xavrx::system::HollywoodCpu::Starlet,xavrx::system::HollywoodTimer::alarm_register,4,2U),"Starlet timer alarm configured");
  system->advance_starlet_ticks(2U);
  check((system->hollywood().interrupts().flags(xavrx::system::HollywoodCpu::Starlet)&1U)!=0U,"system scheduler advances Starlet timer");
  emulator.reset();
  check(system->memory().read32(xavrx::system::WiiMemory::mem1_cached,word)==xavrx::memory::AccessResult::Success&&word==0U,"system reset clears Wii RAM");
  check(system->hollywood().timer().counter()==0U&&!system->hollywood().timer().armed(),"system reset clears Hollywood timer");
 }
 if(failures){std::cerr<<failures<<" emulator integration test(s) failed\n";return 1;}
 std::cout<<"All XAVRX-WII emulator integration tests passed\n";return 0;
}
