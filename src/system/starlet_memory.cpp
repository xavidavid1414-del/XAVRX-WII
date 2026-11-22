#include "xavrx/system/starlet_memory.hpp"
namespace xavrx::system {
namespace { constexpr std::uint32_t sm_bit=1U<<5U, broadway_sram_bit=1U<<3U, boot0_disable_bit=1U<<12U; }
StarletMemory::StarletMemory() noexcept = default;
bool StarletMemory::CpuView::read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept {
 return owner_.read_as(cpu_,base_+offset,width,value);
}
bool StarletMemory::CpuView::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept {
 return owner_.write_as(cpu_,base_+offset,width,value);
}
bool StarletMemory::map_into(WiiMemory& bus) noexcept {
 if(!bus.map_device_dual(sram_base,0x20000U,broadway_sram_,starlet_sram_))return false;
 return bus.map_device_dual(mirror_base,mirror_size,broadway_mirror_,starlet_mirror_);
}
bool StarletMemory::load_boot_rom(const std::array<std::uint8_t,boot_rom_size>& image) noexcept {
 boot_rom_=image;boot_rom_loaded_=true;return true;
}
StarletMemory::Translation StarletMemory::translate(std::uint32_t address) const noexcept {
 enum class Window : std::uint8_t { Main, Fffe, Ffff };
 Window window=Window::Main;std::uint32_t offset=0;
 if(address>=sram_base&&address<sram_base+0x20000U)offset=address-sram_base;
 else if(address>=mirror_base&&static_cast<std::uint64_t>(address)<static_cast<std::uint64_t>(mirror_base)+mirror_size) {
  const auto relative=address-mirror_base;
  if(relative<0xe0000U)offset=relative&0x1ffffU;
  else if(relative<0xf0000U){window=Window::Fffe;offset=relative-0xe0000U;}
  else {window=Window::Ffff;offset=relative-0xf0000U;}
 } else return {};
 const bool sm=(srnprot_&sm_bit)!=0U;
 const bool boot_disabled=(boot0_&boot0_disable_bit)!=0U;
 if(window==Window::Main) {
  if(!boot_disabled&&!sm) {
   if(offset<0x10000U)return {Region::SramA,offset};
   if(offset<0x18000U)return {Region::SramB,offset-0x10000U};
   return {};
  }
  if(!boot_disabled&&sm)return {Region::BootRom,offset%boot_rom_size};
  if(boot_disabled&&!sm) {
   if(offset<0x10000U)return {Region::SramA,offset};
   if(offset<0x18000U)return {Region::SramB,offset-0x10000U};
   return {};
  }
  if(offset<0x8000U)return {Region::SramB,offset};
  if(offset>=0x10000U&&offset<0x20000U)return {Region::SramA,offset-0x10000U};
  return {};
 }
 if(window==Window::Fffe) {
  if(!boot_disabled)return sm?Translation{Region::BootRom,offset%boot_rom_size}:Translation{Region::SramA,offset};
  if(!sm) {
   if(offset<0x10000U)return {Region::SramA,offset};
   if(offset<0x18000U)return {Region::SramB,offset-0x10000U};
  } else {
   if(offset<0x8000U)return {Region::SramB,offset};
   if(offset>=0x10000U)return {Region::SramA,offset-0x10000U};
  }
  return {};
 }
 if(!boot_disabled)return sm?Translation{Region::SramA,offset}:Translation{Region::BootRom,offset%boot_rom_size};
 if(!sm)return offset<0x10000U?Translation{Region::SramA,offset}:Translation{};
 if(offset<0x8000U)return {Region::SramB,offset};
 if(offset>=0x10000U)return {Region::SramA,offset-0x10000U};
 return {};
}
bool StarletMemory::read_region(Translation m,std::uint8_t width,std::uint64_t& value) const noexcept {
 if(m.region==Region::None||(m.region==Region::BootRom&&!boot_rom_loaded_))return false;
 const std::uint8_t* bytes=nullptr;std::size_t size=0;
 switch(m.region){case Region::SramA:bytes=sram_a_.data();size=sram_a_.size();break;case Region::SramB:bytes=sram_b_.data();size=sram_b_.size();break;case Region::BootRom:bytes=boot_rom_.data();size=boot_rom_.size();break;default:return false;}
 if(static_cast<std::uint64_t>(m.offset)+width>size)return false;
 std::uint64_t result=0;for(std::uint8_t i=0;i<width;++i)result=(result<<8U)|bytes[m.offset+i];value=result;return true;
}
bool StarletMemory::write_region(Translation m,std::uint8_t width,std::uint64_t value) noexcept {
 if(m.region==Region::None||m.region==Region::BootRom)return false;
 std::uint8_t* bytes=nullptr;std::size_t size=0;
 if(m.region==Region::SramA){bytes=sram_a_.data();size=sram_a_.size();}
 else if(m.region==Region::SramB){bytes=sram_b_.data();size=sram_b_.size();}
 else return false;
 if(static_cast<std::uint64_t>(m.offset)+width>size)return false;
 for(std::uint8_t i=0;i<width;++i){const auto shift=static_cast<unsigned>((width-1U-i)*8U);bytes[m.offset+i]=static_cast<std::uint8_t>(value>>shift);}return true;
}
bool StarletMemory::read_as(StarletMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t& value) noexcept {
 if((width!=1U&&width!=2U&&width!=4U&&width!=8U)||(address&(width-1U))!=0U)return false;
 if(cpu==StarletMemoryCpu::Broadway&&(srnprot_&broadway_sram_bit)==0U)return false;
 return read_region(translate(address),width,value);
}
bool StarletMemory::write_as(StarletMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t value) noexcept {
 if((width!=1U&&width!=2U&&width!=4U&&width!=8U)||(address&(width-1U))!=0U)return false;
 if(cpu==StarletMemoryCpu::Broadway&&(srnprot_&broadway_sram_bit)==0U)return false;
 return write_region(translate(address),width,value);
}
bool StarletMemory::read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept {
 return read_as(StarletMemoryCpu::Broadway,sram_base+offset,width,value);
}
bool StarletMemory::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept {
 return write_as(StarletMemoryCpu::Broadway,sram_base+offset,width,value);
}
void StarletMemory::reset() noexcept {sram_a_.fill(0);sram_b_.fill(0);srnprot_=0;boot0_=0;}
}
