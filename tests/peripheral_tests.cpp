#include "xavrx/system/peripherals.hpp"
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;
 NandInterface nand;SdHostController sd;UsbHostController usb;std::uint64_t value=0;
 check(nand.aperture_size()==0x1cU&&!nand.little_endian(),"NAND aperture and big-endian bus");
 check(nand.write(0x00U,4,0x12345678U)&&nand.read(0x00U,4,value)&&value==0x12345678U,"NAND register readback");
 check(!nand.read(0x18U,4,value),"unknown NAND register remains unmapped");
 check(!nand.write(0x04U,2,0x1234U),"NAND rejects non-word access");
 check(sd.aperture_size()==0x200U&&sd.little_endian(),"SD host aperture and reversed little-endian register view");
 check(sd.write(0x08U,4,0x12345678U)&&sd.read(0x08U,4,value)&&value==0x12345678U,"SD host endian conversion round trip");
 check(!sd.read(0x64U,4,value),"unlisted SD host offset rejected");
 check(usb.aperture_size()==0x100U&&usb.little_endian(),"USB EHCI aperture and reversed little-endian register view");
 check(usb.write(0x10U,4,0xaabbccddU)&&usb.read(0x10U,4,value)&&value==0xaabbccddU,"USB EHCI register endian conversion");
 check(!usb.read(0x30U,4,value),"unlisted USB register rejected");
 nand.reset();check(nand.read(0x00U,4,value)&&value==0U,"NAND reset clears register storage");
 sd.reset();check(sd.read(0x08U,4,value)&&value==0U,"SD reset clears register storage");
 usb.reset();check(usb.read(0x10U,4,value)&&value==0U,"USB reset clears register storage");
 if(failures){std::cerr<<failures<<" peripheral test(s) failed\n";return 1;}
 std::cout<<"All peripheral register-window tests passed\n";return 0;
}
