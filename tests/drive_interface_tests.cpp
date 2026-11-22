#include "xavrx/system/peripherals.hpp"
#include "xavrx/system/wii_system.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
namespace {int failures=0;void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;DriveInterface di;std::uint64_t value=0;
 check(di.read(0x00U,4U,value)&&value==0U,"DI DISR resets clear");
 check(di.read(0x04U,4U,value)&&(value&1U)!=0U,"DI cover register reports open with no disc");
 check(di.write(0x04U,4U,0U)&&di.read(0x04U,4U,value)&&(value&1U)!=0U,"DI cover state is read-only");
 di.signal_cover_interrupt();check(di.read(0x04U,4U,value)&&(value&7U)==7U&&!di.interrupt_pending(),"DI cover interrupt latches while masked");
 check(di.write(0x04U,4U,2U)&&di.interrupt_pending(),"DI cover interrupt asserts when enabled");
 check(di.write(0x04U,4U,4U)&&di.read(0x04U,4U,value)&&(value&4U)==0U&&!di.interrupt_pending(),"DI cover interrupt clears write-one-to-clear");
 check(di.mount_disc_image(std::vector<std::uint8_t>(64U,0x5aU))&&di.read(0x04U,4U,value)&&(value&1U)==0U,"DI cover state reports closed with mounted disc");
 check(di.write(0x00U,4U,(1U<<1U)|(1U<<3U)|(1U<<5U)),"DI DISR enables device-error transfer-complete and break masks");
 di.signal_transfer_complete();check(di.read(0x00U,4U,value)&&(value&(1U<<4U))!=0U&&di.interrupt_pending(),"DI transfer-complete flag latches and asserts masked IRQ");
 check(di.write(0x00U,4U,(1U<<1U)|(1U<<3U)|(1U<<5U)|(1U<<4U))&&di.read(0x00U,4U,value)&&(value&(1U<<4U))==0U,"DI transfer-complete flag clears with W1C while retaining masks");
 di.signal_device_error();check(di.interrupt_pending(),"DI device-error flag asserts through its independent mask");
 check(di.write(0x00U,4U,(1U<<1U)|(1U<<3U)|(1U<<5U)|(1U<<2U))&&!di.interrupt_pending(),"DI device-error flag clears with W1C");
 di.signal_break_interrupt();check(di.interrupt_pending(),"DI break flag asserts through its independent mask");
 check(di.write(0x00U,4U,(1U<<1U)|(1U<<3U)|(1U<<5U)|(1U<<6U))&&!di.interrupt_pending(),"DI break flag clears with W1C");
 check(di.write(0x1cU,4U,0xffffffffU)&&di.read(0x1cU,4U,value)&&value==6U,"DI DMA control stores only DMA and direction bits; TSTART self-clears");
 check(!di.write(0x00U,2U,0U)&&!di.read(0x04U,2U,value),"DI status registers reject non-word accesses");
 di.signal_transfer_complete();(void)di.write(0x00U,4U,1U<<3U);di.reset();
 check(di.read(0x00U,4U,value)&&value==0U&&!di.interrupt_pending(),"DI reset clears status and masks");
 check(di.read(0x04U,4U,value)&&(value&7U)==0U,"DI reset preserves mounted-disc cover state and clears cover interrupt mask");
 check(di.read(0x1cU,4U,value)&&value==0U,"DI reset clears DMA control");
 // End-to-end DI DMA must update the same status register visible through MMIO.
 WiiSystem system;check(system.initialize(),"WiiSystem maps DI registers");
 const auto release_di=(1U<<10U)|(1U<<3U);
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d000194U,4U,release_di)==xavrx::memory::AccessResult::Success,"release DI reset gates");
 check(system.drive_interface().mount_disc_image(std::vector<std::uint8_t>(32U,0x6dU)),"mount image for integrated DI transfer");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d806000U,4U,1U<<3U)==xavrx::memory::AccessResult::Success,"enable DI transfer-complete status mask");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d806008U,4U,0xa8000000U)==xavrx::memory::AccessResult::Success,"configure integrated DI read command");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d80600cU,4U,0U)==xavrx::memory::AccessResult::Success,"configure integrated DI source offset");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d806010U,4U,32U)==xavrx::memory::AccessResult::Success,"configure integrated DI command length");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d806014U,4U,0x80004000U)==xavrx::memory::AccessResult::Success,"configure integrated DI destination");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d806018U,4U,32U)==xavrx::memory::AccessResult::Success,"configure integrated DI DMA length");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4U,3U)==xavrx::memory::AccessResult::Success,"start integrated DI read DMA");
 check(system.memory().read_as(WiiMemoryCpu::Starlet,0x0d806000U,4U,value)==xavrx::memory::AccessResult::Success&&(value&(1U<<4U))!=0U&&system.drive_interface().interrupt_pending()&&(system.hollywood().interrupts().flags(HollywoodCpu::Broadway)&(1U<<18U))!=0U&&(system.hollywood().interrupts().flags(HollywoodCpu::Starlet)&(1U<<18U))!=0U,"successful DI DMA latches TCINT and routes Hollywood IRQ 18 to both CPUs");
 check(system.memory().write_as(WiiMemoryCpu::Starlet,0x0d806000U,4U,(1U<<3U)|(1U<<4U))==xavrx::memory::AccessResult::Success&&!system.drive_interface().interrupt_pending(),"MMIO write clears integrated DI completion interrupt");
 check((system.hollywood().interrupts().flags(HollywoodCpu::Broadway)&(1U<<18U))!=0U&&(system.hollywood().interrupts().flags(HollywoodCpu::Starlet)&(1U<<18U))!=0U,"Hollywood DI IRQ flags remain latched until CPU acknowledgement");
 system.hollywood().interrupts().clear(HollywoodCpu::Broadway,1U<<18U);
 system.hollywood().interrupts().clear(HollywoodCpu::Starlet,1U<<18U);
 check((system.hollywood().interrupts().flags(HollywoodCpu::Broadway)&(1U<<18U))==0U&&(system.hollywood().interrupts().flags(HollywoodCpu::Starlet)&(1U<<18U))==0U,"Broadway and Starlet independently acknowledge DI IRQ 18");
 if(failures){std::cerr<<failures<<" DI register test(s) failed\n";return 1;}
 std::cout<<"All DI register tests passed\n";return 0;
}
