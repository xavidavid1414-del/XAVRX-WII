#include "xavrx/system/wii_memory.hpp"
namespace xavrx::system {
WiiMemory::WiiMemory():mem1_(mem1_size),mem2_(mem2_size){}
bool WiiMemory::translate(std::uint32_t address,std::uint8_t width,std::vector<std::uint8_t>*& bank,std::size_t& offset)noexcept{
 if(width!=1U&&width!=2U&&width!=4U&&width!=8U)return false;
 if((address&(static_cast<std::uint32_t>(width)-1U))!=0U)return false;
 const auto a=static_cast<std::uint64_t>(address),w=static_cast<std::uint64_t>(width);
 const auto map=[&](std::uint32_t base,std::size_t size,std::vector<std::uint8_t>& target)->bool{
  if(a<base||a+w>static_cast<std::uint64_t>(base)+size)return false;
  bank=&target;offset=static_cast<std::size_t>(a-base);return true;
 };
 return map(mem1_cached,mem1_size,mem1_)||map(mem1_uncached,mem1_size,mem1_)||
        map(mem1_physical,mem1_size,mem1_)||map(mem2_cached,mem2_size,mem2_)||
        map(mem2_uncached,mem2_size,mem2_)||map(mem2_physical,mem2_size,mem2_);
}
memory::AccessResult WiiMemory::read(std::uint32_t address,std::uint8_t width,std::uint64_t& value)noexcept{
 if(width!=1U&&width!=2U&&width!=4U&&width!=8U)return memory::AccessResult::InvalidWidth;
 if((address&(static_cast<std::uint32_t>(width)-1U))!=0U)return memory::AccessResult::Misaligned;
 std::vector<std::uint8_t>* bank=nullptr;std::size_t offset=0;
 if(!translate(address,width,bank,offset))return memory::AccessResult::Unmapped;
 std::uint64_t result=0;for(std::uint8_t i=0;i<width;++i)result=(result<<8U)|(*bank)[offset+i];value=result;return memory::AccessResult::Success;
}
memory::AccessResult WiiMemory::write(std::uint32_t address,std::uint8_t width,std::uint64_t value)noexcept{
 if(width!=1U&&width!=2U&&width!=4U&&width!=8U)return memory::AccessResult::InvalidWidth;
 if((address&(static_cast<std::uint32_t>(width)-1U))!=0U)return memory::AccessResult::Misaligned;
 std::vector<std::uint8_t>* bank=nullptr;std::size_t offset=0;
 if(!translate(address,width,bank,offset))return memory::AccessResult::Unmapped;
 for(std::uint8_t i=0;i<width;++i){const auto shift=static_cast<unsigned>((width-1U-i)*8U);(*bank)[offset+i]=static_cast<std::uint8_t>(value>>shift);}
 return memory::AccessResult::Success;
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
