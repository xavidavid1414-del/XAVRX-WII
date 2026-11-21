#include "xavrx/memory/memory.hpp"
#include <limits>
namespace xavrx::memory {
namespace { constexpr std::uint64_t address_space_size=0x100000000ULL; constexpr bool supported_width(std::uint8_t w) noexcept{return w==1U||w==2U||w==4U||w==8U;} }
Memory::Memory(std::uint32_t base,std::size_t size):ram_base_(base){
 const auto end=static_cast<std::uint64_t>(base)+static_cast<std::uint64_t>(size);
 if(size==0U||size>std::numeric_limits<std::uint32_t>::max()||end>address_space_size)return;
 ram_.resize(size);valid_=true;
}
bool Memory::contains(std::uint32_t base,std::uint32_t size,std::uint32_t address,std::uint8_t width)const noexcept{
 return address>=base&&static_cast<std::uint64_t>(address)+width<=static_cast<std::uint64_t>(base)+size;
}
bool Memory::map_device(std::uint32_t base,std::uint32_t size,MemoryMappedDevice& device)noexcept{
 if(!valid_||size==0U||static_cast<std::uint64_t>(base)+size>address_space_size)return false;
 const auto end=static_cast<std::uint64_t>(base)+size,ram_end=static_cast<std::uint64_t>(ram_base_)+ram_.size();
 if(static_cast<std::uint64_t>(base)<ram_end&&end>ram_base_)return false;
 for(const auto& r:devices_){const auto e=static_cast<std::uint64_t>(r.base)+r.size;if(static_cast<std::uint64_t>(base)<e&&end>r.base)return false;}
 try{devices_.push_back({base,size,&device});}catch(...){return false;}return true;
}
AccessResult Memory::read(std::uint32_t a,std::uint8_t w,std::uint64_t& v)noexcept{
 if(!valid_)return AccessResult::InvalidConfiguration;if(!supported_width(w))return AccessResult::InvalidWidth;
 if((a&(static_cast<std::uint32_t>(w)-1U))!=0U)return AccessResult::Misaligned;
 const auto end=static_cast<std::uint64_t>(ram_base_)+ram_.size();
 if(a>=ram_base_&&static_cast<std::uint64_t>(a)+w<=end){const auto off=static_cast<std::size_t>(a-ram_base_);std::uint64_t x=0;for(std::uint8_t i=0;i<w;++i)x=(x<<8U)|ram_[off+i];v=x;return AccessResult::Success;}
 for(const auto& r:devices_)if(contains(r.base,r.size,a,w)){std::uint64_t x=0;if(!r.device->read(a-r.base,w,x))return AccessResult::DeviceError;v=x;return AccessResult::Success;}
 return AccessResult::Unmapped;
}
AccessResult Memory::write(std::uint32_t a,std::uint8_t w,std::uint64_t v)noexcept{
 if(!valid_)return AccessResult::InvalidConfiguration;if(!supported_width(w))return AccessResult::InvalidWidth;
 if((a&(static_cast<std::uint32_t>(w)-1U))!=0U)return AccessResult::Misaligned;
 const auto end=static_cast<std::uint64_t>(ram_base_)+ram_.size();
 if(a>=ram_base_&&static_cast<std::uint64_t>(a)+w<=end){const auto off=static_cast<std::size_t>(a-ram_base_);for(std::uint8_t i=0;i<w;++i){const auto shift=static_cast<unsigned>((w-1U-i)*8U);ram_[off+i]=static_cast<std::uint8_t>(v>>shift);}return AccessResult::Success;}
 for(const auto& r:devices_)if(contains(r.base,r.size,a,w))return r.device->write(a-r.base,w,v)?AccessResult::Success:AccessResult::DeviceError;
 return AccessResult::Unmapped;
}
AccessResult Memory::read8(std::uint32_t a,std::uint8_t& v)noexcept{std::uint64_t x=0;auto r=read(a,1,x);if(r==AccessResult::Success)v=static_cast<std::uint8_t>(x);return r;}
AccessResult Memory::read16(std::uint32_t a,std::uint16_t& v)noexcept{std::uint64_t x=0;auto r=read(a,2,x);if(r==AccessResult::Success)v=static_cast<std::uint16_t>(x);return r;}
AccessResult Memory::read32(std::uint32_t a,std::uint32_t& v)noexcept{std::uint64_t x=0;auto r=read(a,4,x);if(r==AccessResult::Success)v=static_cast<std::uint32_t>(x);return r;}
AccessResult Memory::read64(std::uint32_t a,std::uint64_t& v)noexcept{return read(a,8,v);}
AccessResult Memory::write8(std::uint32_t a,std::uint8_t v)noexcept{return write(a,1,v);}
AccessResult Memory::write16(std::uint32_t a,std::uint16_t v)noexcept{return write(a,2,v);}
AccessResult Memory::write32(std::uint32_t a,std::uint32_t v)noexcept{return write(a,4,v);}
AccessResult Memory::write64(std::uint32_t a,std::uint64_t v)noexcept{return write(a,8,v);}
}
