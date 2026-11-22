#include "xavrx/system/peripherals.hpp"
namespace xavrx::system {
namespace {
constexpr std::uint32_t nand_offsets[]={0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U};
constexpr std::uint32_t sd_offsets[]={
 0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U,0x18U,0x1cU,0x20U,0x24U,0x28U,0x2cU,
 0x30U,0x34U,0x38U,0x3cU,0x40U,0x44U,0x48U,0x50U,0x54U,0x58U,0x5cU,0x60U
};
constexpr std::uint32_t usb_offsets[]={
 0x00U,0x04U,0x08U,0x0cU,0x10U,0x14U,0x18U,0x1cU,0x20U,0x24U,0x28U,
 0x34U,0x38U,0x3cU,0x40U,0x44U,0x48U,0x4cU,0x50U,0x54U,0x90U,0x94U,
 0x98U,0x9cU,0xa0U,0xa4U,0xa8U,0xacU,0xb0U,0xb4U,0xc4U,0xc8U,0xccU,
 0xd0U,0xd4U,0xd8U,0xdcU,0xe0U,0xe4U,0xe8U,0xecU,0xf0U,0xf4U,0xf8U,0xfcU
};
}
RegisterWindow::RegisterWindow(std::uint32_t aperture,const std::uint32_t* offsets,std::size_t count,bool little) noexcept
 :aperture_size_(aperture),little_endian_(little){
 if(offsets==nullptr)return;
 for(std::size_t i=0;i<count;++i){const auto off=offsets[i];if((off&3U)==0U&&off<aperture_size_&&off/4U<word_count)implemented_[off/4U]=true;}
}
std::uint32_t RegisterWindow::byte_swap(std::uint32_t v) noexcept{
 return ((v&0x000000ffU)<<24U)|((v&0x0000ff00U)<<8U)|((v&0x00ff0000U)>>8U)|((v&0xff000000U)>>24U);
}
bool RegisterWindow::supports(std::uint32_t off)const noexcept{return (off&3U)==0U&&off<aperture_size_&&off/4U<word_count&&implemented_[off/4U];}
bool RegisterWindow::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{
 if(width!=4U||!supports(off))return false;
 const auto raw=registers_[off/4U];value=little_endian_?byte_swap(raw):raw;return true;
}
bool RegisterWindow::write(std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{
 if(width!=4U||value>0xffffffffULL||!supports(off))return false;
 const auto raw=static_cast<std::uint32_t>(value);registers_[off/4U]=little_endian_?byte_swap(raw):raw;return true;
}
void RegisterWindow::reset()noexcept{registers_.fill(0U);}
NandInterface::NandInterface()noexcept:RegisterWindow(0x1cU,nand_offsets,sizeof(nand_offsets)/sizeof(nand_offsets[0]),false){}
SdHostController::SdHostController()noexcept:RegisterWindow(0x200U,sd_offsets,sizeof(sd_offsets)/sizeof(sd_offsets[0]),true){}
UsbHostController::UsbHostController()noexcept:RegisterWindow(0x100U,usb_offsets,sizeof(usb_offsets)/sizeof(usb_offsets[0]),true){}
}
