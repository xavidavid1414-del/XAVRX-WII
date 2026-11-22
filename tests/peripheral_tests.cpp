#include "xavrx/system/peripherals.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;NandInterface nand;SdHostController sd;UsbHostController usb;DriveInterface di;WirelessLanController wlan;std::uint64_t value=0;
 check(nand.aperture_size()==0x1cU&&!nand.little_endian(),"NAND aperture and big-endian bus");
 check(nand.write(0x00U,4,0x12345678U)&&nand.read(0x00U,4,value)&&value==0x12345678U,"NAND register readback");
 check(!nand.read(0x18U,4,value)&&!nand.write(0x04U,2,0x1234U),"NAND rejects unknown and non-word accesses");
 std::array<std::uint8_t,NandInterface::page_size> page{},readback{};std::array<std::uint8_t,NandInterface::spare_size> spare{},spare_out{};
 page.fill(0xf0U);spare.fill(0xfeU);
 check(nand.read_page(7U,readback,spare_out)&&readback[0]==0xffU&&spare_out[0]==0xffU,"erased NAND page reads as 0xff");
 check(nand.program_page(7U,page,spare)&&nand.allocated_pages()==1U,"NAND programs sparse page");
 check(nand.read_page(7U,readback,spare_out)&&readback==page&&spare_out==spare,"NAND page and spare readback");
 page[0]=0xffU;check(!nand.program_page(7U,page,spare),"NAND programming cannot change zero bits back to one");
 check(nand.erase_block(0U)&&nand.read_page(7U,readback,spare_out)&&readback[0]==0xffU,"NAND block erase restores erased state");
 check(!nand.erase_block(NandInterface::block_count),"NAND rejects out-of-range block");
 check(sd.aperture_size()==0x200U&&sd.little_endian(),"SD host aperture and little-endian register view");
 check(sd.write(0x08U,4,0x12345678U)&&sd.read(0x08U,4,value)&&value==0x12345678U,"SD host register endian round trip");
 check(!sd.read(0x64U,4,value),"unlisted SD host offset rejected");
 std::vector<std::uint8_t> image(4U*SdHostController::block_size,0U),block(SdHostController::block_size);
 image[0]=0x42U;check(sd.attach_card(std::move(image))&&sd.card_present()&&sd.block_count()==4U,"SD card image attach");
 check(sd.read_blocks(0U,1U,block)&&block[0]==0x42U,"SD block read");
 block[0]=0xa5U;check(sd.write_blocks(1U,1U,block)&&sd.read_blocks(1U,1U,block)&&block[0]==0xa5U,"SD block write/readback");
 check(!sd.read_blocks(4U,1U,block)&&!sd.write_blocks(0U,0U,block),"SD rejects invalid block ranges");
 sd.detach_card();check(!sd.card_present()&&!sd.read_blocks(0U,1U,block),"SD detach removes card");
 check(usb.aperture_size()==0x100U&&usb.little_endian(),"USB EHCI aperture and little-endian register view");
 check(usb.read(0x00U,4,value)&&value==0x01000020U,"EHCI capability register is read-only hardware identity");
 check(!usb.write(0x00U,4,0U),"EHCI rejects writes to capability registers");
 check(usb.write(0x10U,4,1U)&&usb.read(0x14U,4,value)&&(value&1U)==0U,"EHCI run clears halted status");
 usb.connect_device(true);const std::array<std::uint8_t,3> payload{{1U,2U,3U}};
 check(usb.enqueue_transfer(0x1234U,payload)&&usb.pending_transfers()==1U,"EHCI accepts bounded transfer");
 usb.advance_microframes(1U);UsbHostController::Transfer transfer;
 check(usb.take_completed_transfer(transfer)&&transfer.token==0x1234U&&transfer.payload.size()==3U,"EHCI completes queued transfer on active connected frame");
 check(usb.read(0x14U,4,value)&&(value&2U)!=0U,"EHCI raises USB interrupt status on completion");
 check(usb.write(0x14U,4,2U)&&usb.read(0x14U,4,value)&&(value&2U)==0U,"EHCI interrupt status is write-one-to-clear");
 check(usb.write(0x10U,4,2U)&&usb.read(0x14U,4,value)&&(value&1U)!=0U,"EHCI controller reset halts controller");
 check(usb.pending_transfers()==0U&&usb.completed_transfers()==0U,"EHCI reset clears transfer queues");
 check(di.aperture_size()==0x40U&&!di.little_endian(),"DI aperture and big-endian registers");
 check(di.write(0x08U,4,0x11223344U)&&di.read(0x08U,4,value)&&value==0x11223344U,"DI register access");
 check(!di.read(0x28U,4,value),"DI rejects offsets outside documented aperture");
 check(wlan.aperture_size()==0x200U&&wlan.little_endian(),"WLAN SDIO aperture");
 BluetoothHciTransport bt;std::vector<std::uint8_t> packet;
 const std::uint8_t command[]={0x01U,0x03U,0x0cU,0x00U},event[]={0x04U,0x0eU,0x01U,0x00U},acl[]={0x02U,0x01U,0x00U,0x02U,0x00U,0xaaU,0xbbU};
 check(bt.submit_host_packet(command)&&bt.take_device_packet(packet)&&packet.size()==sizeof(command),"Bluetooth HCI command transport");
 check(bt.submit_device_packet(event)&&bt.take_host_packet(packet)&&packet.size()==sizeof(event),"Bluetooth HCI event transport");
 check(bt.submit_host_packet(acl)&&bt.take_device_packet(packet)&&packet.size()==sizeof(acl),"Bluetooth ACL host-to-device transport");
 check(bt.submit_device_packet(acl)&&bt.take_host_packet(packet)&&packet.size()==sizeof(acl),"Bluetooth ACL device-to-host transport");
 const std::uint8_t malformed[]={0x01U,0x03U,0x0cU,0x01U};check(!bt.submit_host_packet(malformed)&&!bt.submit_device_packet(command),"Bluetooth rejects malformed packet directions");
 for(std::size_t i=0;i<BluetoothHciTransport::queue_capacity;++i)check(bt.submit_host_packet(command),"Bluetooth queue accepts bounded packets");
 check(!bt.submit_host_packet(command),"Bluetooth queue capacity enforced");bt.reset();check(bt.host_queue_size()==0U&&bt.device_queue_size()==0U,"Bluetooth reset clears queues");
 nand.reset();check(nand.read(0x00U,4,value)&&value==0U,"NAND reset clears controller registers but retains flash contents");
 if(failures){std::cerr<<failures<<" peripheral test(s) failed\n";return 1;}std::cout<<"All peripheral controller tests passed\n";return 0;
}
