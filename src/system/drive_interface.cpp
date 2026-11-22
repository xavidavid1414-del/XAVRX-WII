#include "xavrx/system/peripherals.hpp"
namespace xavrx::system {
bool DriveInterface::read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept {
 if(width!=4U)return false;
 switch(offset){
 case 0x00U:value=disr_;return true;
 case 0x04U:value=(disc_image_.empty()?1U:0U)|(dicvr_&0x6U);return true;
 case 0x1cU:value=dicr_&0x6U;return true;
 default:return RegisterWindow::read(offset,width,value);
 }
}
bool DriveInterface::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept {
 if(width!=4U||value>0xffffffffULL)return false;
 const auto v=static_cast<std::uint32_t>(value);
 switch(offset){
 case 0x00U:
  disr_=(disr_&~((1U<<1U)|(1U<<3U)|(1U<<5U)))|(v&((1U<<1U)|(1U<<3U)|(1U<<5U)));
  if((v&(1U<<2U))!=0U)disr_&=~(1U<<2U);
  if((v&(1U<<4U))!=0U)disr_&=~(1U<<4U);
  if((v&(1U<<6U))!=0U)disr_&=~(1U<<6U);
  return true;
 case 0x04U:
  dicvr_=(dicvr_&~(1U<<1U))|(v&(1U<<1U));
  if((v&(1U<<2U))!=0U)dicvr_&=~(1U<<2U);
  return true;
 case 0x1cU:
  dicr_=v&0x6U;
  return true;
 default:return RegisterWindow::write(offset,width,value);
 }
}
bool DriveInterface::interrupt_pending() const noexcept {
 return ((disr_&(1U<<2U))!=0U&&(disr_&(1U<<1U))!=0U)||
        ((disr_&(1U<<4U))!=0U&&(disr_&(1U<<3U))!=0U)||
        ((disr_&(1U<<6U))!=0U&&(disr_&(1U<<5U))!=0U)||
        ((dicvr_&(1U<<2U))!=0U&&(dicvr_&(1U<<1U))!=0U);
}
void DriveInterface::reset() noexcept {
 RegisterWindow::reset();
 disr_=0U;dicvr_=0U;dicr_=0U;
}
}
