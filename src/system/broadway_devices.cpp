#include "xavrx/system/broadway_devices.hpp"
namespace xavrx::system {
bool DspMailboxInterface::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value) noexcept {
 if(width!=2U)return false;
 switch(off){
 case 0x00U: value=in_high_;return true;
 case 0x02U: value=in_low_;return true;
 case 0x04U: value=static_cast<std::uint16_t>((out_high_&0x7fffU)|(output_pending_?0x8000U:0U));return true;
 case 0x06U: value=out_low_;output_pending_=false;return true;
 default:return false;
 }
}
bool DspMailboxInterface::write(std::uint32_t off,std::uint8_t width,std::uint64_t value) noexcept {
 if(width!=2U||value>0xffffU)return false;
 switch(off){
 case 0x00U: in_high_=static_cast<std::uint16_t>(value);return true;
 case 0x02U:
  in_low_=static_cast<std::uint16_t>(value);
  input_pending_=(in_high_&0x8000U)!=0U;
  return true;
 default:return false;
 }
}
bool DspMailboxInterface::take_mail_to_dsp(std::uint32_t& value) noexcept {
 if(!input_pending_)return false;
 value=(static_cast<std::uint32_t>(in_high_&0x7fffU)<<16U)|in_low_;
 input_pending_=false;in_high_&=0x7fffU;return true;
}
bool DspMailboxInterface::post_mail_from_dsp(std::uint32_t value) noexcept {
 if(output_pending_)return false;
 out_high_=static_cast<std::uint16_t>(((value>>16U)&0x7fffU)|0x8000U);
 out_low_=static_cast<std::uint16_t>(value);output_pending_=true;return true;
}
void DspMailboxInterface::reset() noexcept {in_high_=in_low_=out_high_=out_low_=0;input_pending_=output_pending_=false;}
bool ProcessorInterruptInterface::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value) noexcept {
 if(width!=4U)return false;
 if(off==0x00U){value=cause_;return true;}
 if(off==0x04U){value=mask_;return true;}
 return false;
}
bool ProcessorInterruptInterface::write(std::uint32_t off,std::uint8_t width,std::uint64_t value) noexcept {
 if(width!=4U||value>0xffffffffULL)return false;
 if(off==0x00U){cause_&=~static_cast<std::uint32_t>(value);return true;}
 if(off==0x04U){mask_=static_cast<std::uint32_t>(value)&0x00007fffU;return true;}
 return false;
}
void ProcessorInterruptInterface::raise(std::uint8_t source) noexcept {
 if(source<15U)cause_|=(std::uint32_t{1}<<source);
}
bool GxWriteGatherPort::write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept {
 if(offset!=0U||width!=4U||value>0xffffffffULL)return false;
 const auto word=static_cast<std::uint32_t>(value);
 return processor_.submit({&word,1U});
}
}
