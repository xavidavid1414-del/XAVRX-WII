#include "xavrx/system/peripherals.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;
 NandInterface nand;SdHostController sd;UsbHostController usb;DriveInterface di;WirelessLanController wlan;std::uint64_t value=0;
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
 check(di.aperture_size()==0x40U&&!di.little_endian(),"DI aperture and big-endian registers");
 check(di.write(0x08U,4,0x11223344U)&&di.read(0x08U,4,value)&&value==0x11223344U,"DI command buffer register access");
 check(di.write(0x24U,4,0x55667788U)&&di.read(0x24U,4,value)&&value==0x55667788U,"DI config register access");
 check(!di.read(0x28U,4,value),"DI rejects offsets outside documented Wii aperture");
 check(wlan.aperture_size()==0x200U&&wlan.little_endian(),"WLAN SDIO aperture and reversed little-endian registers");
 check(wlan.write(0x24U,4,0x00010000U)&&wlan.read(0x24U,4,value)&&value==0x00010000U,"WLAN present-state register access");
 check(!wlan.read(0x200U,4,value),"WLAN rejects out-of-aperture access");
 BluetoothHciTransport bt;std::vector<std::uint8_t> packet;
 const std::uint8_t command[]={0x01U,0x03U,0x0cU,0x00U};
 const std::uint8_t event[]={0x04U,0x0eU,0x01U,0x00U};
 const std::uint8_t acl[]={0x02U,0x01U,0x00U,0x02U,0x00U,0xaaU,0xbbU};
 check(bt.submit_host_packet(command)&&bt.take_device_packet(packet)&&packet.size()==sizeof(command)&&packet[0]==0x01U,"Bluetooth HCI command transport");
 check(bt.submit_device_packet(event)&&bt.take_host_packet(packet)&&packet.size()==sizeof(event)&&packet[0]==0x04U,"Bluetooth HCI event transport");
 check(bt.submit_host_packet(acl)&&bt.take_device_packet(packet)&&packet.size()==sizeof(acl),"Bluetooth host-to-device ACL transport");
 check(bt.submit_device_packet(acl)&&bt.take_host_packet(packet)&&packet.size()==sizeof(acl),"Bluetooth device-to-host ACL transport");
 const std::uint8_t malformed[]={0x01U,0x03U,0x0cU,0x01U};
 check(!bt.submit_host_packet(malformed),"Bluetooth rejects malformed HCI command length");
 check(!bt.submit_device_packet(command),"Bluetooth rejects host-only command packet from device");
 for(std::size_t i=0;i<BluetoothHciTransport::queue_capacity;++i)check(bt.submit_host_packet(command),"Bluetooth queue accepts packet within capacity");
 check(!bt.submit_host_packet(command),"Bluetooth queue enforces capacity");
 bt.reset();check(bt.host_queue_size()==0U&&bt.device_queue_size()==0U,"Bluetooth reset clears both packet queues");
 nand.reset();check(nand.read(0x00U,4,value)&&value==0U,"NAND reset clears register storage");
 sd.reset();check(sd.read(0x08U,4,value)&&value==0U,"SD reset clears register storage");
 usb.reset();check(usb.read(0x10U,4,value)&&value==0U,"USB reset clears register storage");
 if(failures){std::cerr<<failures<<" peripheral test(s) failed\n";return 1;}
 std::cout<<"All peripheral register-window and HCI transport tests passed\n";return 0;
}
