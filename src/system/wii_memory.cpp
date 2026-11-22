#include "xavrx/system/wii_memory.hpp"
#include <array>
#include <utility>
namespace xavrx::system {
namespace {
constexpr std::uint64_t address_space_size=0x100000000ULL;
constexpr bool supported_width(std::uint8_t width)noexcept{return width==1U||width==2U||width==4U||width==8U;}
constexpr bool overlaps(std::uint64_t a0,std::uint64_t a1,std::uint64_t b0,std::uint64_t b1)noexcept{return a0<b1&&a1>b0;}
}
WiiMemory::WiiMemory():mem1_(mem1_size),mem2_(mem2_size){}
bool WiiMemory::map_device_dual(std::uint32_t base,std::uint32_t size,memory::MemoryMappedDevice& broadway,memory::MemoryMappedDevice& starlet)noexcept{
 if(size==0U||static_cast<std::uint64_t>(base)+size>address_space_size)return false;
 const auto begin=static_cast<std::uint64_t>(base),end=begin+size;
 const std::array<std::pair<std::uint32_t,std::size_t>,6> banks{{
  {mem1_cached,mem1_size},{mem1_uncached,mem1_size},{mem1_physical,mem1_size},
  {mem2_cached,mem2_size},{mem2_uncached,mem2_size},{mem2_physical,mem2_size}
 }};
 for(const auto& bank:banks)if(overlaps(begin,end,bank.first,static_cast<std::uint64_t>(bank.first)+bank.second))return false;
 for(const auto& region:devices_)if(overlaps(begin,end,region.base,static_cast<std::uint64_t>(region.base)+region.size))return false;
 try{devices_.push_back({base,size,&broadway,&starlet});}catch(...){return false;}
 return true;
}
bool WiiMemory::translate(std::uint32_t address,std::uint8_t width,std::vector<std::uint8_t>*& bank,std::size_t& offset)noexcept{
 if(!supported_width(width)||(address&(static_cast<std::uint32_t>(width)-1U))!=0U)return false;
 const auto a=static_cast<std::uint64_t>(address),w=static_cast<std::uint64_t>(width);
 const auto map=[&](std::uint32_t base,std::size_t size,std::vector<std::uint8_t>& target)->bool{
  if(a<base||a+w>static_cast<std::uint64_t>(base)+size)return false;
  bank=&target;offset=static_cast<std::size_t>(a-base);return true;
 };
 return map(mem1_cached,mem1_size,mem1_)||map(mem1_uncached,mem1_size,mem1_)||
        map(mem1_physical,mem1_size,mem1_)||map(mem2_cached,mem2_size,mem2_)||
        map(mem2_uncached,mem2_size,mem2_)||map(mem2_physical,mem2_size,mem2_);
}
memory::AccessResult WiiMemory::read(std::uint32_t address,std::uint8_t width,std::uint64_t& value)noexcept{return read_as(WiiMemoryCpu::Broadway,address,width,value);}
memory::AccessResult WiiMemory::write(std::uint32_t address,std::uint8_t width,std::uint64_t value)noexcept{return write_as(WiiMemoryCpu::Broadway,address,width,value);}
memory::AccessResult WiiMemory::read_as(WiiMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t& value)noexcept{
 if(!supported_width(width))return memory::AccessResult::InvalidWidth;
 if((address&(static_cast<std::uint32_t>(width)-1U))!=0U)return memory::AccessResult::Misaligned;
 std::vector<std::uint8_t>* bank=nullptr;std::size_t offset=0;
 if(translate(address,width,bank,offset)){
  std::uint64_t result=0;for(std::uint8_t i=0;i<width;++i)result=(result<<8U)|(*bank)[offset+i];value=result;return memory::AccessResult::Success;
 }
 for(const auto& region:devices_){
  if(address<region.base||static_cast<std::uint64_t>(address)+width>static_cast<std::uint64_t>(region.base)+region.size)continue;
  auto* device=cpu==WiiMemoryCpu::Broadway?region.broadway:region.starlet;
  std::uint64_t result=0;if(!device->read(address-region.base,width,result))return memory::AccessResult::DeviceError;
  value=result;return memory::AccessResult::Success;
 }
 return memory::AccessResult::Unmapped;
}
memory::AccessResult WiiMemory::write_as(WiiMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t value)noexcept{
 if(!supported_width(width))return memory::AccessResult::InvalidWidth;
 if((address&(static_cast<std::uint32_t>(width)-1U))!=0U)return memory::AccessResult::Misaligned;
 std::vector<std::uint8_t>* bank=nullptr;std::size_t offset=0;
 if(translate(address,width,bank,offset)){
  for(std::uint8_t i=0;i<width;++i){const auto shift=static_cast<unsigned>((width-1U-i)*8U);(*bank)[offset+i]=static_cast<std::uint8_t>(value>>shift);}
  return memory::AccessResult::Success;
 }
 for(const auto& region:devices_){
  if(address<region.base||static_cast<std::uint64_t>(address)+width>static_cast<std::uint64_t>(region.base)+region.size)continue;
  auto* device=cpu==WiiMemoryCpu::Broadway?region.broadway:region.starlet;
  return device->write(address-region.base,width,value)?memory::AccessResult::Success:memory::AccessResult::DeviceError;
 }
 return memory::AccessResult::Unmapped;
}
memory::AccessResult WiiMemory::read8(std::uint32_t a,std::uint8_t& v)noexcept{std::uint64_t x=0;auto r=read(a,1,x);if(r==memory::AccessResult::Success)v=static_cast<std::uint8_t>(x);return r;}
memory::AccessResult WiiMemory::read16(std::uint32_t a,std::uint16_t& v)noexcept{std::uint64_t x=0;auto r=read(a,2,x);if(r==memory::AccessResult::Success)v=static_cast<std::uint16_t>(x);return r;}
memory::AccessResult WiiMemory::read32(std::uint32_t a,std::uint32_t& v)noexcept{std::uint64_t x=0;auto r=read(a,4,x);if(r==memory::AccessResult::Success)v=static_cast<std::uint32_t>(x);return r;}
memory::AccessResult WiiMemory::read64(std::uint32_t a,std::uint64_t& v)noexcept{return read(a,8,v);}
memory::AccessResult WiiMemory::write8(std::uint32_t a,std::uint8_t v)noexcept{return write(a,1,v);}
memory::AccessResult WiiMemory::write16(std::uint32_t a,std::uint16_t v)noexcept{return write(a,2,v);}
memory::AccessResult WiiMemory::write32(std::uint32_t a,std::uint32_t v)noexcept{return write(a,4,v);}
memory::AccessResult WiiMemory::write64(std::uint32_t a,std::uint64_t v)noexcept{return write(a,8,v);}
void WiiMemory::reset()noexcept{for(auto& b:mem1_)b=0;for(auto& b:mem2_)b=0;}
}
