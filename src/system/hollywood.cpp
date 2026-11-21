#include "xavrx/system/hollywood.hpp"
namespace xavrx::system {
namespace { constexpr std::uint32_t pins=0x00ffffffU; constexpr std::uint32_t bit(std::uint8_t n) noexcept{return std::uint32_t{1}<<n;} }
void HollywoodInterrupts::raise(std::uint8_t source,IrqRoute route) noexcept {
 if(source>=32U)return;
 const auto b=bit(source);const auto route_bits=static_cast<std::uint8_t>(route);
 if((route_bits&1U)!=0U)ppc_flags_|=b;
 if((route_bits&2U)!=0U)arm_flags_|=b;
}
void HollywoodInterrupts::clear(HollywoodCpu cpu,std::uint32_t bits) noexcept {if(cpu==HollywoodCpu::Broadway)ppc_flags_&=~bits;else arm_flags_&=~bits;}
void HollywoodInterrupts::set_mask(HollywoodCpu cpu,std::uint32_t bits) noexcept {if(cpu==HollywoodCpu::Broadway)ppc_mask_=bits;else arm_mask_=bits;}
std::uint32_t HollywoodInterrupts::flags(HollywoodCpu cpu)const noexcept{return cpu==HollywoodCpu::Broadway?ppc_flags_:arm_flags_;}
std::uint32_t HollywoodInterrupts::mask(HollywoodCpu cpu)const noexcept{return cpu==HollywoodCpu::Broadway?ppc_mask_:arm_mask_;}
bool HollywoodInterrupts::asserted(HollywoodCpu cpu)const noexcept{return (flags(cpu)&mask(cpu))!=0U;}
bool HollywoodIpc::read(HollywoodCpu cpu,std::uint32_t off,std::uint32_t& value)const noexcept {
 switch(off){case ppc_msg:value=ppc_message_;return true;case ppc_ctrl:value=(x1_?1U:0U)|(y2_?2U:0U)|(y1_?4U:0U)|(x2_?8U:0U)|(iy1_?16U:0U)|(iy2_?32U:0U);return true;
 case arm_msg:value=arm_message_;return true;case arm_ctrl:if(cpu!=HollywoodCpu::Starlet)return false;value=(y1_?1U:0U)|(x2_?2U:0U)|(x1_?4U:0U)|(y2_?8U:0U)|(ix1_?16U:0U)|(ix2_?32U:0U);return true;default:return false;}
}
bool HollywoodIpc::write(HollywoodCpu cpu,std::uint32_t off,std::uint32_t value)noexcept{
 switch(off){
 case ppc_msg:ppc_message_=value;return true;
 case arm_msg:arm_message_=value;return true;
 case ppc_ctrl:
  x1_=static_cast<std::uint8_t>(value&1U);x2_=static_cast<std::uint8_t>((value>>3U)&1U);
  if((value&2U)!=0U)y2_=0;if((value&4U)!=0U)y1_=0;
  iy1_=static_cast<std::uint8_t>((value>>4U)&1U);iy2_=static_cast<std::uint8_t>((value>>5U)&1U);
  if((value&4U)!=0U&&iy1_)irq_.raise(30,IrqRoute::Both);if((value&2U)!=0U&&iy2_)irq_.raise(30,IrqRoute::Both);
  update_interrupts();return true;
 case arm_ctrl:
  if(cpu!=HollywoodCpu::Starlet)return false;
  y1_=static_cast<std::uint8_t>(value&1U);y2_=static_cast<std::uint8_t>((value>>3U)&1U);
  if((value&2U)!=0U)x2_=0;if((value&4U)!=0U)x1_=0;
  ix1_=static_cast<std::uint8_t>((value>>4U)&1U);ix2_=static_cast<std::uint8_t>((value>>5U)&1U);
  if((value&4U)!=0U&&ix1_)irq_.raise(31,IrqRoute::Both);if((value&2U)!=0U&&ix2_)irq_.raise(31,IrqRoute::Both);
  update_interrupts();return true;
 default:return false;
 }
}
void HollywoodIpc::update_interrupts()noexcept{
 if(y1_&&iy1_)irq_.raise(30,IrqRoute::Both);if(y2_&&iy2_)irq_.raise(30,IrqRoute::Both);
 if(x1_&&ix1_)irq_.raise(31,IrqRoute::Both);if(x2_&&ix2_)irq_.raise(31,IrqRoute::Both);
}
void HollywoodIpc::reset()noexcept{ppc_message_=arm_message_=0;x1_=x2_=y1_=y2_=iy1_=iy2_=ix1_=ix2_=0;}
bool HollywoodGpio::read(HollywoodCpu cpu,std::uint32_t off,std::uint32_t& value)const noexcept{
 const bool b=off>=0xc0U&&off<=0xd8U;const auto reg=off-(b?0xc0U:0xe0U);
 if(b&&cpu!=HollywoodCpu::Broadway)return false;if(!b&&cpu!=HollywoodCpu::Starlet)return false;
 switch(reg){case 0:value=b?b_out_:a_out_;break;case 4:value=b?b_dir_:a_dir_;break;case 8:value=input_;break;case 12:value=b?b_level_:a_level_;break;case 16:value=b?b_flags_:a_flags_;break;case 20:value=b?b_mask_:a_mask_;break;case 24:value=b?straps_:straps_;break;case 28:if(b)return false;value=enable_;break;case 32:if(b)return false;value=a_out_;break;case 36:if(b)return false;value=a_dir_;break;case 40:if(b)return false;value=input_;break;case 44:if(b)return false;value=a_level_;break;case 48:if(b)return false;value=a_flags_;break;case 52:if(b)return false;value=a_mask_;break;case 56:if(b)return false;value=straps_;break;case 60:if(b)return false;value=owner_;break;default:return false;}
 value&=pins;return true;
}
bool HollywoodGpio::write(HollywoodCpu cpu,std::uint32_t off,std::uint32_t value)noexcept{
 const bool b=off>=0xc0U&&off<=0xd8U;const auto reg=off-(b?0xc0U:0xe0U);
 if(b&&cpu!=HollywoodCpu::Broadway)return false;if(!b&&cpu!=HollywoodCpu::Starlet)return false;
 if(b){const auto allowed=owner_&pins;switch(reg){case 0:b_out_=(b_out_&~allowed)|(value&allowed);break;case 4:b_dir_=(b_dir_&~allowed)|(value&allowed);break;case 12:b_level_=(b_level_&~allowed)|(value&allowed);break;case 16:b_flags_&=~(value&allowed);break;case 20:b_mask_=(b_mask_&~allowed)|(value&allowed);break;default:return false;}}
 else {switch(reg){case 28:enable_=value&pins;break;case 32:a_out_=((a_out_&owner_)|(value&~owner_))&pins;break;case 36:a_dir_=((a_dir_&owner_)|(value&~owner_))&pins;break;case 44:a_level_=((a_level_&owner_)|(value&~owner_))&pins;break;case 48:a_flags_&=~(value&~owner_&pins);break;case 52:a_mask_=((a_mask_&owner_)|(value&~owner_))&pins;break;case 60:owner_=value&pins;break;default:return false;}}
 refresh_interrupts();return true;
}
void HollywoodGpio::set_inputs(std::uint32_t levels)noexcept{input_=levels&pins;refresh_interrupts();}
void HollywoodGpio::refresh_interrupts()noexcept{
 const auto b_active=(~(input_^b_level_))&b_mask_&owner_&pins;
 const auto a_active=(~(input_^a_level_))&a_mask_&~owner_&enable_&pins;
 if(b_active){b_flags_|=b_active;irq_.raise(10,IrqRoute::Both);}if(a_active){a_flags_|=a_active;irq_.raise(11,IrqRoute::Both);}
}
void HollywoodGpio::reset()noexcept{input_=straps_=owner_=b_out_=b_dir_=b_level_=b_flags_=b_mask_=a_out_=a_dir_=a_level_=a_flags_=a_mask_=0;enable_=pins;}
bool HollywoodHardware::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{return read_as(HollywoodCpu::Broadway,off,width,value);}
bool HollywoodHardware::write(std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{return write_as(HollywoodCpu::Broadway,off,width,value);}
bool HollywoodHardware::read_as(HollywoodCpu cpu,std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{
 if(width!=4U||(off&3U)!=0U||off>=window_size)return false;std::uint32_t v=0;
 if(off<0x10U){if(!ipc_.read(cpu,off,v))return false;}
 else if(off>=0x30U&&off<=0x3cU){if((off>=0x38U)&&cpu!=HollywoodCpu::Starlet)return false;switch(off){case 0x30:v=interrupts_.flags(HollywoodCpu::Broadway);break;case 0x34:v=interrupts_.mask(HollywoodCpu::Broadway);break;case 0x38:v=interrupts_.flags(HollywoodCpu::Starlet);break;case 0x3c:v=interrupts_.mask(HollywoodCpu::Starlet);break;}}
 else if(off>=0xc0U&&off<=0xfcU){if(!gpio_.read(cpu,off,v))return false;}
 else return false;value=v;return true;
}
bool HollywoodHardware::write_as(HollywoodCpu cpu,std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{
 if(width!=4U||(off&3U)!=0U||off>=window_size||value>0xffffffffULL)return false;const auto v=static_cast<std::uint32_t>(value);
 if(off<0x10U)return ipc_.write(cpu,off,v);
 if(off>=0x30U&&off<=0x3cU){if((off>=0x38U)&&cpu!=HollywoodCpu::Starlet)return false;switch(off){case 0x30:interrupts_.clear(HollywoodCpu::Broadway,v);break;case 0x34:interrupts_.set_mask(HollywoodCpu::Broadway,v);break;case 0x38:interrupts_.clear(HollywoodCpu::Starlet,v);break;case 0x3c:interrupts_.set_mask(HollywoodCpu::Starlet,v);break;}return true;}
 if(off>=0xc0U&&off<=0xfcU)return gpio_.write(cpu,off,v);return false;
}
void HollywoodHardware::reset()noexcept{interrupts_=HollywoodInterrupts{};ipc_.reset();gpio_.reset();}
}
