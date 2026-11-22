#include "xavrx/system/peripherals.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
namespace {int failures=0;void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;DriveInterface di;std::uint64_t value=0;
 check(di.read(0x00U,4U,value)&&value==0U,"DI DISR resets clear");
 check(di.read(0x04U,4U,value)&&(value&1U)!=0U,"DI cover register reports open with no disc");
 check(di.write(0x04U,4U,2U)&&di.read(0x04U,4U,value)&&(value&3U)==3U,"DI cover interrupt mask is writable and cover state is read-only");
 di.signal_cover_interrupt();check(di.read(0x04U,4U,value)&&(value&7U)==7U&&!di.interrupt_pending(),"DI cover interrupt latches while masked");
 check(di.write(0x04U,4U,6U)&&di.interrupt_pending(),"DI cover interrupt asserts when enabled");
 check(di.write(0x04U,4U,4U)&&di.read(0x04U,4U,value)&&(value&4U)==0U&&!di.interrupt_pending(),"DI cover interrupt clears write-one-to-clear");
 check(di.mount_disc_image(std::vector<std::uint8_t>(64U,0x5aU))&&di.read(0x04U,4U,value)&&(value&1U)==0U,"DI cover state reports closed with mounted disc");
 check(di.write(0x00U,4U,(1U<<1U)|(1U<<3U)|(1U<<5U)),"DI DISR enables device-error transfer-complete and break masks");
 di.signal_transfer_complete();check(di.read(0x00U,4U,value)&&(value&(1U<<4U))!=0U&&di.interrupt_pending(),"DI transfer-complete flag latches and asserts masked IRQ");
 check(di.write(0x00U,4U,1U<<4U)&&di.read(0x00U,4U,value)&&(value&(1U<<4U))==0U,"DI transfer-complete flag clears with W1C");
 di.signal_device_error();check(di.interrupt_pending(),"DI device-error flag asserts through its independent mask");
 check(di.write(0x00U,4U,1U<<2U)&&!di.interrupt_pending(),"DI device-error flag clears with W1C");
 di.signal_break_interrupt();check(di.interrupt_pending(),"DI break flag asserts through its independent mask");
 check(di.write(0x00U,4U,1U<<6U)&&!di.interrupt_pending(),"DI break flag clears with W1C");
 check(di.write(0x1cU,4U,0xffffffffU)&&di.read(0x1cU,4U,value)&&value==6U,"DI DMA control stores only DMA and direction bits; TSTART self-clears");
 check(!di.write(0x00U,2U,0U)&&!di.read(0x04U,2U,value),"DI status registers reject non-word accesses");
 di.signal_transfer_complete();di.write(0x00U,4U,1U<<3U);di.reset();
 check(di.read(0x00U,4U,value)&&value==0U&&!di.interrupt_pending(),"DI reset clears status and masks");
 check(di.read(0x04U,4U,value)&&(value&7U)==1U,"DI reset preserves physical no-disc cover state and clears cover interrupt mask");
 check(di.read(0x1cU,4U,value)&&value==0U,"DI reset clears DMA control");
 if(failures){std::cerr<<failures<<" DI register test(s) failed\n";return 1;}
 std::cout<<"All DI register tests passed\n";return 0;
}
