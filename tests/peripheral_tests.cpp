#include "xavrx/system/peripherals.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;NandInterface nand;SdHostController sd;UsbHostController usb;DriveInterface di;WirelessLanController wlan;std::uint64_t value=0;
 check(nand.aperture_size()==0x1cU&&!nand.little_endian(),"NAND aperture and big-endian bus");
 check(nand.write(0x00U,4,0x12345678U)&&nand.read(0x00U,4,value)&&value==0x12345678U,"NAND register readback");
 check(nand.write(0x18U,4,0x12345678U)&&nand.read(0x18U,4,value)&&value==0x12345678U,"NAND exposes documented opaque NAND_UNK register without assigning semantics");
 check(!nand.read(0x1cU,4,value)&&!nand.write(0x04U,2,0x1234U),"NAND rejects out-of-aperture and non-word accesses");
 std::array<std::uint8_t,NandInterface::page_size> page{},readback{};std::array<std::uint8_t,NandInterface::spare_size> spare{},spare_out{};
 page.fill(0xf0U);spare.fill(0xfeU);
 check(nand.read_page(7U,readback,spare_out)&&readback[0]==0xffU&&spare_out[0]==0xffU,"erased NAND page reads as 0xff");
 check(nand.program_page(7U,page,spare)&&nand.allocated_pages()==1U,"NAND programs sparse page");
 check(nand.read_page(7U,readback,spare_out)&&readback==page&&spare_out==spare,"NAND page and spare readback");
 page[0]=0xffU;check(!nand.program_page(7U,page,spare),"NAND programming cannot change zero bits back to one");
 check(nand.erase_block(0U)&&nand.read_page(7U,readback,spare_out)&&readback[0]==0xffU,"NAND block erase restores erased state");
 check(!nand.erase_block(NandInterface::block_count),"NAND rejects out-of-range block");
 std::array<std::uint8_t,NandInterface::page_size> second_page{},second_read{};
 std::array<std::uint8_t,NandInterface::spare_size> second_spare{},second_spare_read{};
 second_page.fill(0x7fU);second_spare.fill(0x3fU);
 const auto last_page=static_cast<std::uint32_t>(NandInterface::block_count*NandInterface::pages_per_block-1U);
 check(nand.program_page(last_page,second_page,second_spare)&&nand.read_page(last_page,second_read,second_spare_read)&&second_read==second_page,"NAND accesses final valid page");
 check(!nand.read_page(static_cast<std::uint32_t>(NandInterface::block_count*NandInterface::pages_per_block),second_read,second_spare_read),"NAND rejects page immediately beyond capacity");
 check(nand.program_page(NandInterface::pages_per_block,second_page,second_spare)&&nand.erase_block(0U)&&nand.read_page(NandInterface::pages_per_block,second_read,second_spare_read)&&second_read==second_page,"NAND block erase preserves adjacent block contents");
 check(nand.erase_block(NandInterface::block_count-1U)&&nand.read_page(last_page,second_read,second_spare_read)&&second_read[0]==0xffU,"NAND erases final block boundary");
 check(sd.aperture_size()==0x200U&&sd.little_endian(),"SD host aperture and little-endian register view");
 check(sd.write(0x08U,4,0x12345678U)&&sd.read(0x08U,4,value)&&value==0x12345678U,"SD host register endian round trip");
 check(!sd.read(0x64U,4,value),"unlisted SD host offset rejected");
 std::vector<std::uint8_t> image(4U*SdHostController::block_size,0U),block(SdHostController::block_size);
 image[0]=0x42U;check(sd.attach_card(std::move(image))&&sd.card_present()&&sd.block_count()==4U,"SD card image attach");
 check(sd.read_blocks(0U,1U,block)&&block[0]==0x42U,"SD block read");
 block[0]=0xa5U;check(sd.write_blocks(1U,1U,block)&&sd.read_blocks(1U,1U,block)&&block[0]==0xa5U,"SD block write/readback");
 check(!sd.read_blocks(4U,1U,block)&&!sd.write_blocks(0U,0U,block),"SD rejects invalid block ranges");
 check(!sd.attach_card({})&&!sd.attach_card(std::vector<std::uint8_t>(SdHostController::block_size-1U)),"SD rejects empty and partial-block card images");
 std::vector<std::uint8_t> final_block(SdHostController::block_size,0x6cU);
 check(sd.write_blocks(3U,1U,final_block)&&sd.read_blocks(3U,1U,block)&&block==final_block,"SD accesses final valid block");
 check(!sd.read_blocks(0U,1U,std::span<std::uint8_t>{})&&!sd.write_blocks(0U,1U,std::span<const std::uint8_t>{}),"SD rejects buffers whose size does not match block count");
 check(!sd.read_blocks(0U,0x00800000U,std::span<std::uint8_t>{})&&!sd.write_blocks(0U,0x00800000U,std::span<const std::uint8_t>{}),"SD rejects huge block counts without byte-size overflow");
 sd.detach_card();check(!sd.card_present()&&!sd.read_blocks(0U,1U,block),"SD detach removes card");
 check(usb.aperture_size()==0x100U&&usb.little_endian(),"USB EHCI aperture and little-endian register view");
 check(usb.read(0x00U,4,value)&&value==0x01000020U,"EHCI capability register is read-only hardware identity");
 check(!usb.write(0x00U,4,0U),"EHCI rejects writes to capability registers");
 check(usb.write(0x10U,4,1U)&&usb.read(0x14U,4,value)&&(value&(1U<<12U))==0U,"EHCI run clears halted status");
 usb.connect_device(true);
 check(usb.read(0x54U,4,value)&&(value&0x3U)==0x3U&&(value&0x4U)==0U,"EHCI connection sets CCS and CSC without prematurely enabling port");
 check(usb.write(0x54U,4,0x2U)&&usb.read(0x54U,4,value)&&(value&0x2U)==0U&&(value&0x1U)!=0U,"EHCI PORTSC clears connect-change using write-one-to-clear");
 check(usb.write(0x54U,4,0x100U)&&usb.complete_port_reset(true)&&usb.read(0x54U,4,value)&&(value&0x4U)!=0U,"EHCI high-speed port reset completion enables connected port");
 const std::array<std::uint8_t,3> payload{{1U,2U,3U}};
 check(usb.enqueue_transfer(0x1234U,payload)&&usb.pending_transfers()==1U,"EHCI accepts bounded transfer");
 usb.advance_microframes(1U);UsbHostController::Transfer transfer;
 check(usb.take_completed_transfer(transfer)&&transfer.token==0x1234U&&transfer.payload.size()==3U,"EHCI completes queued transfer on active connected frame");
 check(usb.enqueue_transfer(0x2222U,payload),"EHCI accepts transfer before disconnect test");
 usb.connect_device(false);usb.advance_microframes(2U);
 check(usb.pending_transfers()==1U&&usb.completed_transfers()==0U,"EHCI does not complete transfers while port is disconnected");
 usb.connect_device(true);check(usb.write(0x54U,4,0x100U)&&usb.complete_port_reset(true),"EHCI reconnect requires high-speed port reset completion");(void)usb.write(0x10U,4,0U);usb.advance_microframes(1U);
 check(usb.pending_transfers()==1U,"EHCI does not complete transfers while controller is halted");
 (void)usb.write(0x10U,4,1U);usb.advance_microframes(1U);
 check(usb.take_completed_transfer(transfer)&&transfer.token==0x2222U,"EHCI resumes queued transfer after reconnect and run");
 for(std::size_t i=0;i<UsbHostController::transfer_capacity;++i)check(usb.enqueue_transfer(static_cast<std::uint32_t>(i),payload),"EHCI pending queue accepts capacity");
 check(!usb.enqueue_transfer(0xffffffffU,payload),"EHCI rejects transfer beyond pending queue capacity");
 usb.advance_microframes(static_cast<std::uint32_t>(UsbHostController::transfer_capacity));
 check(usb.pending_transfers()==0U&&usb.completed_transfers()==UsbHostController::transfer_capacity,"EHCI moves bounded pending queue into completion queue");
 check(!usb.enqueue_transfer(0xffffffffU,payload),"EHCI backpressure applies while completion queue is full");
 for(std::size_t i=0;i<UsbHostController::transfer_capacity;++i)check(usb.take_completed_transfer(transfer),"EHCI completion queue drains in bounded order");
 for(std::size_t i=0;i+1U<UsbHostController::transfer_capacity;++i){check(usb.enqueue_transfer(static_cast<std::uint32_t>(0x1000U+i),payload),"EHCI fills completion queue to one slot remaining");usb.advance_microframes(1U);}
 for(std::size_t i=0;i<UsbHostController::transfer_capacity;++i)check(usb.enqueue_transfer(static_cast<std::uint32_t>(0x2000U+i),payload),"EHCI queues pending transfers while completion queue is almost full");
 usb.advance_microframes(static_cast<std::uint32_t>(UsbHostController::transfer_capacity));
 check(usb.completed_transfers()==UsbHostController::transfer_capacity&&usb.pending_transfers()==UsbHostController::transfer_capacity-1U,"EHCI never overfills completion queue");
 for(std::size_t i=0;i<UsbHostController::transfer_capacity;++i)check(usb.take_completed_transfer(transfer),"EHCI drains exactly one full completion queue");
 usb.advance_microframes(static_cast<std::uint32_t>(UsbHostController::transfer_capacity));
 check(usb.pending_transfers()==0U&&usb.completed_transfers()==UsbHostController::transfer_capacity-1U,"EHCI resumes pending completions after backpressure clears");
 for(std::size_t i=0;i+1U<UsbHostController::transfer_capacity;++i)check(usb.take_completed_transfer(transfer),"EHCI drains remaining completions after backpressure");
 check(usb.enqueue_transfer(0xabcdefU,payload),"EHCI accepts transfers after completion queue drains");

 check(usb.read(0x14U,4,value)&&(value&1U)!=0U,"EHCI raises USB interrupt status on completion");
 check(usb.write(0x14U,4,1U)&&usb.read(0x14U,4,value)&&(value&1U)==0U,"EHCI interrupt status is write-one-to-clear");
 check(usb.write(0x10U,4,2U)&&usb.read(0x14U,4,value)&&(value&(1U<<12U))!=0U,"EHCI controller reset halts controller");
 check(usb.pending_transfers()==0U&&usb.completed_transfers()==0U,"EHCI reset clears transfer queues");
 check(di.aperture_size()==0x40U&&!di.little_endian(),"DI aperture and big-endian registers");
 check(di.write(0x08U,4,0x11223344U)&&di.read(0x08U,4,value)&&value==0x11223344U,"DI register access");
 check(!di.read(0x28U,4,value),"DI rejects offsets outside documented aperture");
 std::vector<std::uint8_t> disc(4096U,0U);disc[0]=0x5aU;disc[4095]=0xa5U;
 check(di.mount_disc_image(std::move(disc))&&di.disc_present()&&di.disc_size()==4096U,"DI mounts read-only disc image");
 std::array<std::uint8_t,2> disc_data{};check(di.read_disc_bytes(0U,disc_data)&&disc_data[0]==0x5aU,"DI reads image bytes");
 check(di.read_disc_bytes(4094U,disc_data)&&disc_data[1]==0xa5U,"DI reads final image bytes");
 check(!di.read_disc_bytes(4095U,disc_data)&&di.read_disc_bytes(0U,std::span<std::uint8_t>{}),"DI rejects out-of-range read and handles empty read span safely");
 check(!di.read_disc_bytes(std::numeric_limits<std::uint64_t>::max(),disc_data),"DI rejects maximum 64-bit offset without overflow");
 check(di.read_disc_bytes(4096U,std::span<std::uint8_t>{}),"DI accepts empty read exactly at image end");
 di.eject_disc();check(!di.disc_present()&&!di.read_disc_bytes(0U,disc_data),"DI eject removes image");

 check(wlan.aperture_size()==0x200U&&wlan.little_endian(),"WLAN SDIO aperture");
 check(wlan.write_sdio_byte(0U,0x20U,0x91U)&&wlan.read_sdio_byte(0U,0x20U,block[0])&&block[0]==0x91U,"WLAN SDIO function zero byte access");
 std::array<std::uint8_t,4> sdio_data{{1U,2U,3U,4U}},sdio_out{};
 check(wlan.write_sdio_data(1U,0x100U,sdio_data)&&wlan.read_sdio_data(1U,0x100U,sdio_out)&&sdio_out==sdio_data,"WLAN SDIO function one bulk transfer");
 check(!wlan.read_sdio_byte(2U,0U,block[0])&&!wlan.write_sdio_data(1U,0x1ffffU,sdio_data),"WLAN SDIO rejects invalid function and address range");
 std::array<std::uint8_t,1> last_byte{{0x5aU}},last_out{};
 check(wlan.write_sdio_byte(1U,WirelessLanController::function_address_space-1U,last_byte[0])&&wlan.read_sdio_byte(1U,WirelessLanController::function_address_space-1U,last_out[0])&&last_out[0]==0x5aU,"WLAN SDIO permits access to final valid byte");
 check(wlan.read_sdio_data(1U,WirelessLanController::function_address_space,std::span<std::uint8_t>{})&&wlan.write_sdio_data(1U,WirelessLanController::function_address_space,std::span<const std::uint8_t>{}),"WLAN SDIO accepts empty transfers at one-past-end address");
 check(!wlan.read_sdio_data(1U,WirelessLanController::function_address_space,last_out)&&!wlan.write_sdio_data(1U,WirelessLanController::function_address_space,last_byte),"WLAN SDIO rejects non-empty transfers at one-past-end address");
 wlan.reset();check(wlan.read_sdio_data(1U,0x100U,sdio_out)&&sdio_out[0]==0U,"WLAN reset clears SDIO function memory");

 BluetoothHciTransport bt;std::vector<std::uint8_t> packet;
 const std::uint8_t command[]={0x01U,0x03U,0x0cU,0x00U},event[]={0x04U,0x0eU,0x01U,0x00U},acl[]={0x02U,0x01U,0x00U,0x02U,0x00U,0xaaU,0xbbU};
 check(bt.submit_host_packet(command)&&bt.take_device_packet(packet)&&packet.size()==sizeof(command),"Bluetooth HCI command transport");
 const std::uint8_t reset_command[]={0x01U,0x03U,0x0cU,0x00U},address_command[]={0x01U,0x09U,0x10U,0x00U},unknown_command[]={0x01U,0xffU,0xffU,0x00U};
 check(bt.process_host_command(reset_command,packet)&&packet.size()==7U&&packet[0]==0x04U&&packet[1]==0x0eU&&packet[6]==0U,"Bluetooth HCI Reset command complete event");
 check(bt.submit_host_packet(acl)&&bt.submit_device_packet(event),"Bluetooth queues accept packets before HCI reset");
 check(bt.process_host_command(reset_command,packet)&&bt.host_queue_size()==0U&&bt.device_queue_size()==0U&&packet[6]==0U,"Bluetooth HCI Reset clears pending transport queues and returns success");
 check(bt.process_host_command(address_command,packet)&&packet.size()==13U&&packet[1]==0x0eU&&packet[2]==10U&&packet[3]==1U&&packet[4]==0x09U&&packet[5]==0x10U&&packet[6]==0U&&packet[7]==0x02U&&packet[12]==0x01U,"Bluetooth HCI Read BD_ADDR response fields and address");
 
 check(bt.process_host_command(unknown_command,packet)&&packet.size()==7U&&packet[6]==1U,"Bluetooth HCI unknown command status");
 const std::uint8_t malformed_command[]={0x01U,0x03U,0x0cU,0x01U};check(!bt.process_host_command(malformed_command,packet),"Bluetooth HCI command parser rejects malformed parameter length");

 check(bt.submit_device_packet(event)&&bt.take_host_packet(packet)&&packet.size()==sizeof(event),"Bluetooth HCI event transport");
 check(bt.submit_host_packet(acl)&&bt.take_device_packet(packet)&&packet.size()==sizeof(acl),"Bluetooth ACL host-to-device transport");
 check(bt.submit_device_packet(acl)&&bt.take_host_packet(packet)&&packet.size()==sizeof(acl),"Bluetooth ACL device-to-host transport");
 const std::uint8_t malformed[]={0x01U,0x03U,0x0cU,0x01U};check(!bt.submit_host_packet(malformed)&&!bt.submit_device_packet(command),"Bluetooth rejects malformed packet directions");
 for(std::size_t i=0;i<BluetoothHciTransport::queue_capacity;++i)check(bt.submit_host_packet(command),"Bluetooth queue accepts bounded packets");
 check(!bt.submit_host_packet(command),"Bluetooth queue capacity enforced");for(std::size_t i=0;i<BluetoothHciTransport::queue_capacity;++i)check(bt.submit_device_packet(event),"Bluetooth device-to-host queue accepts capacity");
 check(!bt.submit_device_packet(event),"Bluetooth device-to-host queue capacity enforced");
 bt.reset();check(bt.host_queue_size()==0U&&bt.device_queue_size()==0U,"Bluetooth reset clears queues");
 nand.reset();check(nand.read(0x00U,4,value)&&value==0U,"NAND reset clears controller registers but retains flash contents");
 if(failures){std::cerr<<failures<<" peripheral test(s) failed\n";return 1;}std::cout<<"All peripheral controller tests passed\n";return 0;
}
