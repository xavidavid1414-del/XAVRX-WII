#include "xavrx/system/peripherals.hpp"
#include <algorithm>
#include <limits>
#include <iterator>
namespace xavrx::system {
namespace {
constexpr std::uint32_t nand_offsets[]={0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U,0x18U};
constexpr std::uint32_t sd_offsets[]={0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U,0x18U,0x1cU,0x20U,0x24U,0x28U,0x2cU,0x30U,0x34U,0x38U,0x3cU,0x40U,0x44U,0x48U,0x50U,0x54U,0x58U,0x5cU,0x60U};
constexpr std::uint32_t usb_offsets[]={0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U,0x18U,0x1cU,0x20U,0x24U,0x28U,0x50U,0x54U,0x90U,0x94U,0x98U,0x9cU,0xa0U,0xa4U,0xa8U,0xacU,0xb0U,0xb4U,0xc4U,0xc8U,0xccU,0xd0U,0xd4U,0xd8U,0xdcU,0xe0U,0xe4U,0xe8U,0xecU,0xf0U,0xf4U,0xf8U,0xfcU};
constexpr std::uint32_t di_offsets[]={0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U,0x18U,0x1cU,0x20U,0x24U};
constexpr std::uint8_t hci_command=0x01U,hci_acl=0x02U,hci_event=0x04U;
bool valid_acl(std::span<const std::uint8_t> p) noexcept{return p.size()>=5U&&p[0]==hci_acl&&(static_cast<std::size_t>(p[3])|(static_cast<std::size_t>(p[4])<<8U))==p.size()-5U;}
}
RegisterWindow::RegisterWindow(std::uint32_t aperture,const std::uint32_t* offsets,std::size_t count,bool little) noexcept:aperture_size_(aperture),little_endian_(little){
 if(!offsets)return;for(std::size_t i=0;i<count;++i){const auto off=offsets[i];if((off&3U)==0U&&off<aperture_size_&&off/4U<word_count)implemented_[off/4U]=true;}
}
std::uint32_t RegisterWindow::byte_swap(std::uint32_t v) noexcept{return ((v&0xffU)<<24U)|((v&0xff00U)<<8U)|((v&0xff0000U)>>8U)|((v&0xff000000U)>>24U);}
bool RegisterWindow::supports(std::uint32_t off)const noexcept{return (off&3U)==0U&&off<aperture_size_&&off/4U<word_count&&implemented_[off/4U];}
bool RegisterWindow::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{if(width!=4U||!supports(off))return false;const auto raw=registers_[off/4U];value=little_endian_?byte_swap(raw):raw;return true;}
bool RegisterWindow::write(std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{if(width!=4U||value>0xffffffffULL||!supports(off))return false;const auto raw=static_cast<std::uint32_t>(value);registers_[off/4U]=little_endian_?byte_swap(raw):raw;return true;}
void RegisterWindow::reset()noexcept{registers_.fill(0U);}
NandInterface::NandInterface()noexcept:RegisterWindow(0x1cU,nand_offsets,std::size(nand_offsets),false){}
bool NandInterface::read_page(std::uint32_t page,std::span<std::uint8_t> data,std::span<std::uint8_t> spare)const noexcept{
 if(page>=block_count*pages_per_block||data.size()!=page_size||spare.size()!=spare_size)return false;
 const auto it=pages_.find(page);if(it==pages_.end()){std::fill(data.begin(),data.end(),0xffU);std::fill(spare.begin(),spare.end(),0xffU);return true;}
 std::copy_n(it->second.begin(),page_size,data.begin());std::copy_n(it->second.begin()+page_size,spare_size,spare.begin());return true;
}
bool NandInterface::program_page(std::uint32_t page,std::span<const std::uint8_t> data,std::span<const std::uint8_t> spare){
 if(page>=block_count*pages_per_block||data.size()!=page_size||spare.size()!=spare_size)return false;
 auto it=pages_.find(page);Page next;if(it==pages_.end())next.fill(0xffU);else next=it->second;
 for(std::size_t i=0;i<page_size;++i)if((next[i]&data[i])!=data[i])return false;
 for(std::size_t i=0;i<spare_size;++i)if((next[page_size+i]&spare[i])!=spare[i])return false;
 for(std::size_t i=0;i<page_size;++i)next[i]&=data[i];for(std::size_t i=0;i<spare_size;++i)next[page_size+i]&=spare[i];
 pages_[page]=next;return true;
}
bool NandInterface::erase_block(std::uint32_t block)noexcept{
 if(block>=block_count)return false;const auto first=block*pages_per_block;
 for(std::uint32_t i=0;i<pages_per_block;++i)pages_.erase(first+i);return true;
}
SdHostController::SdHostController()noexcept:RegisterWindow(0x200U,sd_offsets,std::size(sd_offsets),true){}
bool SdHostController::attach_card(std::vector<std::uint8_t> image){if(image.empty()||image.size()%block_size!=0U||image.size()/block_size>std::numeric_limits<std::uint32_t>::max())return false;card_=std::move(image);return true;}
void SdHostController::detach_card()noexcept{card_.clear();}
bool SdHostController::read_blocks(std::uint32_t first,std::uint32_t count,std::span<std::uint8_t> output)const noexcept{
 const auto byte_count=static_cast<std::uint64_t>(count)*block_size;
 if(card_.empty()||count==0U||byte_count!=output.size()||first>block_count()||count>block_count()-first)return false;
 const auto offset=static_cast<std::size_t>(first)*block_size;std::copy_n(card_.begin()+static_cast<std::ptrdiff_t>(offset),output.size(),output.begin());return true;
}
bool SdHostController::write_blocks(std::uint32_t first,std::uint32_t count,std::span<const std::uint8_t> input)noexcept{
 const auto byte_count=static_cast<std::uint64_t>(count)*block_size;
 if(card_.empty()||count==0U||byte_count!=input.size()||first>block_count()||count>block_count()-first)return false;
 const auto offset=static_cast<std::size_t>(first)*block_size;std::copy(input.begin(),input.end(),card_.begin()+static_cast<std::ptrdiff_t>(offset));return true;
}
UsbHostController::UsbHostController()noexcept:RegisterWindow(0x100U,usb_offsets,std::size(usb_offsets),true){}
bool UsbHostController::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{
 if(width!=4U)return false;
 switch(off){case 0x00U:value=0x01000020U;return true;case 0x04U:value=0x00000101U;return true;case 0x08U:value=0x00000006U;return true;case 0x0cU:value=0U;return true;case 0x10U:value=command_;return true;case 0x14U:value=status_;return true;case 0x18U:value=interrupt_enable_;return true;case 0x1cU:value=frame_index_;return true;case 0x24U:value=periodic_base_;return true;case 0x28U:value=async_list_;return true;case 0x50U:value=config_flag_;return true;case 0x54U:value=port_status_;return true;default:return RegisterWindow::read(off,width,value);}
}
bool UsbHostController::write(std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{
 if(width!=4U||value>0xffffffffULL)return false;const auto v=static_cast<std::uint32_t>(value);
 switch(off){
 case 0x00U:case 0x04U:case 0x08U:case 0x0cU:return false;
 case 0x10U:command_=v;if((v&cmd_reset)!=0U){reset();return true;}if((v&cmd_run)!=0U)status_&=~sts_halted;else status_|=sts_halted;return true;
 case 0x14U:status_&=~v;return true;
 case 0x18U:interrupt_enable_=v;return true;
 case 0x1cU:frame_index_=v&0x3fffU;return true;
 case 0x24U:if((v&0xfffU)!=0U)return false;periodic_base_=v;return true;
 case 0x28U:if((v&0x1fU)!=0U)return false;async_list_=v;return true;
 case 0x50U:config_flag_=v&1U;return true;
 case 0x54U:port_status_&=~(v&0x0000002aU);if((v&0x00000100U)!=0U&&(port_status_&0x1U)!=0U)port_status_|=0x00000100U;return true;
 default:return RegisterWindow::write(off,width,value);
 }
}
void UsbHostController::connect_device(bool connected)noexcept{
 const bool was_connected=(port_status_&0x1U)!=0U;
 const bool was_enabled=(port_status_&0x4U)!=0U;
 if(connected==was_connected)return;
 if(connected){port_status_|=0x3U;status_|=sts_port_change;return;}
 port_status_&=~0x5U;port_status_|=0x2U;status_|=sts_port_change;
 if(was_enabled){port_status_&=~0x4U;port_status_|=0x8U;}
}
bool UsbHostController::complete_port_reset(bool high_speed_device)noexcept{
 if((port_status_&0x1U)==0U||(port_status_&0x100U)==0U)return false;
 port_status_&=~0x100U;
 const bool was_enabled=(port_status_&0x4U)!=0U;
 if(high_speed_device)port_status_|=0x4U;else port_status_&=~0x4U;
 if(was_enabled!=((port_status_&0x4U)!=0U)){port_status_|=0x8U;status_|=sts_port_change;}
 return true;
}
bool UsbHostController::enqueue_transfer(std::uint32_t token,std::span<const std::uint8_t> payload){
 if(pending_.size()>=transfer_capacity||completed_.size()>=transfer_capacity)return false;
 pending_.push_back({token,std::vector<std::uint8_t>(payload.begin(),payload.end())});return true;
}
void UsbHostController::advance_microframes(std::uint64_t count)noexcept{
 frame_index_=(frame_index_+static_cast<std::uint32_t>(count&0x3fffU))&0x3fffU;
 if(count==0U||(command_&cmd_run)==0U||(status_&sts_halted)!=0U||(port_status_&0x4U)==0U)return;
 const auto available=transfer_capacity-completed_.size();
 const auto completions=std::min<std::uint64_t>(count,std::min<std::size_t>(pending_.size(),available));
 for(std::uint64_t i=0;i<completions;++i){completed_.push_back(std::move(pending_.front()));pending_.pop_front();}
 if(completions!=0U)status_|=sts_usbint;
}
bool UsbHostController::take_completed_transfer(Transfer& transfer){if(completed_.empty())return false;transfer=std::move(completed_.front());completed_.pop_front();return true;}
void UsbHostController::reset()noexcept{RegisterWindow::reset();command_=0;status_=sts_halted;interrupt_enable_=0;frame_index_=0;periodic_base_=0;async_list_=0;config_flag_=0;port_status_=0;pending_.clear();completed_.clear();}
DriveInterface::DriveInterface()noexcept:RegisterWindow(0x40U,di_offsets,std::size(di_offsets),false){}
bool DriveInterface::mount_disc_image(std::vector<std::uint8_t> image){if(image.empty())return false;disc_image_=std::move(image);return true;}
void DriveInterface::eject_disc()noexcept{disc_image_.clear();}
bool DriveInterface::read_disc_bytes(std::uint64_t offset,std::span<std::uint8_t> output)const noexcept{
 if(disc_image_.empty()||offset>disc_image_.size()||output.size()>disc_image_.size()-static_cast<std::size_t>(offset))return false;
 std::copy_n(disc_image_.begin()+static_cast<std::ptrdiff_t>(offset),output.size(),output.begin());return true;
}
WirelessLanController::WirelessLanController()noexcept:RegisterWindow(0x200U,sd_offsets,std::size(sd_offsets),true){}
bool WirelessLanController::read_sdio_byte(std::uint8_t function,std::uint32_t address,std::uint8_t& value)const noexcept{
 if(function>=function_count||address>=function_address_space)return false;value=functions_[function][address];return true;
}
bool WirelessLanController::write_sdio_byte(std::uint8_t function,std::uint32_t address,std::uint8_t value)noexcept{
 if(function>=function_count||address>=function_address_space)return false;functions_[function][address]=value;return true;
}
bool WirelessLanController::read_sdio_data(std::uint8_t function,std::uint32_t address,std::span<std::uint8_t> output)const noexcept{
 if(function>=function_count||address>function_address_space||output.size()>function_address_space-address)return false;
 std::copy_n(functions_[function].begin()+static_cast<std::ptrdiff_t>(address),output.size(),output.begin());return true;
}
bool WirelessLanController::write_sdio_data(std::uint8_t function,std::uint32_t address,std::span<const std::uint8_t> input)noexcept{
 if(function>=function_count||address>function_address_space||input.size()>function_address_space-address)return false;
 std::copy(input.begin(),input.end(),functions_[function].begin()+static_cast<std::ptrdiff_t>(address));return true;
}
void WirelessLanController::reset()noexcept{RegisterWindow::reset();for(auto& function:functions_)function.fill(0U);}
bool BluetoothHciTransport::process_host_command(std::span<const std::uint8_t> command,std::vector<std::uint8_t>& event){
 if(command.size()<4U||command[0]!=hci_command||static_cast<std::size_t>(command[3])!=command.size()-4U)return false;
 const std::uint16_t opcode=static_cast<std::uint16_t>(command[1])|static_cast<std::uint16_t>(static_cast<std::uint16_t>(command[2])<<8U);
 std::uint8_t status=0U;std::vector<std::uint8_t> parameters;
 if(opcode==0x0c03U){if(command.size()!=4U)return false;host_to_device_.clear();device_to_host_.clear();}
 else if(opcode==0x1009U){if(command.size()!=4U)return false;parameters={0x02U,0x00U,0x00U,0x00U,0x00U,0x01U};}
 else status=0x01U;
 event.clear();event.reserve(7U+parameters.size());event.push_back(hci_event);event.push_back(0x0eU);
 event.push_back(static_cast<std::uint8_t>(4U+parameters.size()));event.push_back(0x01U);
 event.push_back(command[1]);event.push_back(command[2]);event.push_back(status);
 event.insert(event.end(),parameters.begin(),parameters.end());return true;
}
bool BluetoothHciTransport::valid_host_packet(std::span<const std::uint8_t> p)noexcept{if(p.empty())return false;if(p[0]==hci_command)return p.size()>=4U&&static_cast<std::size_t>(p[3])==p.size()-4U;return valid_acl(p);}
bool BluetoothHciTransport::valid_device_packet(std::span<const std::uint8_t> p)noexcept{if(p.empty())return false;if(p[0]==hci_event)return p.size()>=3U&&static_cast<std::size_t>(p[2])==p.size()-3U;return valid_acl(p);}
bool BluetoothHciTransport::submit_host_packet(std::span<const std::uint8_t> p){if(!valid_host_packet(p)||host_to_device_.size()>=queue_capacity)return false;host_to_device_.emplace_back(p.begin(),p.end());return true;}
bool BluetoothHciTransport::take_device_packet(std::vector<std::uint8_t>& p){if(host_to_device_.empty())return false;p=std::move(host_to_device_.front());host_to_device_.pop_front();return true;}
bool BluetoothHciTransport::submit_device_packet(std::span<const std::uint8_t> p){if(!valid_device_packet(p)||device_to_host_.size()>=queue_capacity)return false;device_to_host_.emplace_back(p.begin(),p.end());return true;}
bool BluetoothHciTransport::take_host_packet(std::vector<std::uint8_t>& p){if(device_to_host_.empty())return false;p=std::move(device_to_host_.front());device_to_host_.pop_front();return true;}
void BluetoothHciTransport::reset()noexcept{host_to_device_.clear();device_to_host_.clear();}
}
