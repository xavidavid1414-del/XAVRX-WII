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

bool HollywoodTimer::write_alarm(std::uint32_t deadline)noexcept{alarm_=deadline;armed_=true;return true;}
void HollywoodTimer::advance(std::uint64_t ticks)noexcept{
 if(ticks==0U)return;
 if(armed_){
  const auto delta=static_cast<std::uint32_t>(alarm_-counter_);
  const std::uint64_t until=delta==0U?(std::uint64_t{1}<<32U):delta;
  if(ticks>=until){armed_=false;irq_.raise(0,IrqRoute::Starlet);}
 }
 counter_+=static_cast<std::uint32_t>(ticks);
}

bool HollywoodSystemControl::read(HollywoodCpu cpu,std::uint32_t off,std::uint32_t& value)const noexcept{
 if(off==ahb_prot_register){value=ahb_prot_;return true;}
 if(cpu!=HollywoodCpu::Starlet)return false;
 if(off==clocks_register){value=clocks_;return true;}
 if(off==resets_register){value=resets_;return true;}
 return false;
}
bool HollywoodSystemControl::engine_access_allowed(HollywoodCpu initiator,HollywoodEngine engine)const noexcept{
 static constexpr std::uint32_t ppc_bits[]={1U<<10U,1U<<11U,1U<<8U,1U<<9U,1U<<7U,1U<<6U,1U<<5U,1U<<4U};
 static constexpr std::uint32_t iop_bits[]={1U<<28U,1U<<29U,1U<<26U,1U<<27U,1U<<25U,1U<<24U,1U<<23U,1U<<22U};
 const auto index=static_cast<std::size_t>(engine);
 if(index>=8U)return false;
 const auto bitmask=initiator==HollywoodCpu::Broadway?ppc_bits[index]:iop_bits[index];
 return (ahb_prot_&bitmask)!=0U;
}
bool HollywoodSystemControl::write(HollywoodCpu cpu,std::uint32_t off,std::uint32_t value)noexcept{
 if(off==ahb_prot_register){if(cpu!=HollywoodCpu::Starlet)return false;ahb_prot_=value;return true;}
 if(cpu!=HollywoodCpu::Starlet)return false;
 if(off==clocks_register){clocks_=value&0x3U;return true;}
 if(off==resets_register){resets_=value&resets_writable_mask;return true;}
 return false;
}

bool HollywoodIpc::read(HollywoodCpu cpu,std::uint32_t off,std::uint32_t& value)const noexcept {
 switch(off){
 case ppc_msg:value=ppc_message_;return true;
 case ppc_ctrl:value=(x1_?1U:0U)|(y2_?2U:0U)|(y1_?4U:0U)|(x2_?8U:0U)|(iy1_?16U:0U)|(iy2_?32U:0U);return true;
 case arm_msg:value=arm_message_;return true;
 case arm_ctrl:if(cpu!=HollywoodCpu::Starlet)return false;value=(y1_?1U:0U)|(x2_?2U:0U)|(x1_?4U:0U)|(y2_?8U:0U)|(ix1_?16U:0U)|(ix2_?32U:0U);return true;
 default:return false;
 }
}
bool HollywoodIpc::write(HollywoodCpu cpu,std::uint32_t off,std::uint32_t value)noexcept{
 switch(off){
 case ppc_msg:ppc_message_=value;return true;
 case arm_msg:arm_message_=value;return true;
 case ppc_ctrl:
  if(cpu!=HollywoodCpu::Broadway)return false;
  x1_=static_cast<std::uint8_t>(value&1U);x2_=static_cast<std::uint8_t>((value>>3U)&1U);
  if((value&2U)!=0U)y2_=0;if((value&4U)!=0U)y1_=0;
  iy1_=static_cast<std::uint8_t>((value>>4U)&1U);iy2_=static_cast<std::uint8_t>((value>>5U)&1U);
  update_interrupts();return true;
 case arm_ctrl:
  if(cpu!=HollywoodCpu::Starlet)return false;
  y1_=static_cast<std::uint8_t>(value&1U);y2_=static_cast<std::uint8_t>((value>>3U)&1U);
  if((value&2U)!=0U)x2_=0;if((value&4U)!=0U)x1_=0;
  ix1_=static_cast<std::uint8_t>((value>>4U)&1U);ix2_=static_cast<std::uint8_t>((value>>5U)&1U);
  update_interrupts();return true;
 default:return false;
 }
}
void HollywoodIpc::update_interrupts()noexcept{
 if((y1_&&iy1_)||(y2_&&iy2_))irq_.raise(30,IrqRoute::Broadway);else irq_.clear(HollywoodCpu::Broadway,bit(30));
 if((x1_&&ix1_)||(x2_&&ix2_))irq_.raise(31,IrqRoute::Starlet);else irq_.clear(HollywoodCpu::Starlet,bit(31));
}
void HollywoodIpc::reset()noexcept{ppc_message_=arm_message_=0;x1_=x2_=y1_=y2_=iy1_=iy2_=ix1_=ix2_=0;}

bool HollywoodGpio::read(HollywoodCpu cpu,std::uint32_t off,std::uint32_t& value)const noexcept{
 if(off==0xdcU){if(cpu!=HollywoodCpu::Starlet)return false;value=enable_;return true;}
 if(off>=0xc0U&&off<=0xd8U){
  switch(off){
   case 0xc0:value=output_&owner_;break;case 0xc4:value=direction_&owner_;break;case 0xc8:value=input_&owner_;break;
   case 0xcc:value=interrupt_level_&owner_;break;case 0xd0:value=interrupt_flags_&owner_;break;case 0xd4:value=interrupt_mask_&owner_;break;case 0xd8:value=straps_&owner_;break;default:return false;
  }
  return true;
 }
 if(off>=0xe0U&&off<=0xfcU){
  if(cpu!=HollywoodCpu::Starlet)return false;
  switch(off){
   case 0xe0:value=output_;break;case 0xe4:value=direction_;break;case 0xe8:value=input_;break;
   case 0xec:value=interrupt_level_;break;case 0xf0:value=interrupt_flags_;break;case 0xf4:value=interrupt_mask_;break;
   case 0xf8:value=straps_;break;case 0xfc:value=owner_;break;default:return false;
  }
  return true;
 }
 return false;
}
bool HollywoodGpio::write(HollywoodCpu cpu,std::uint32_t off,std::uint32_t value)noexcept{
 if(off==0xdcU){if(cpu!=HollywoodCpu::Starlet)return false;enable_=value&pins;refresh_interrupts();return true;}
 if(off>=0xc0U&&off<=0xd8U){
  const auto allowed=owner_&pins;
  switch(off){
   case 0xc0:output_=(output_&~allowed)|(value&allowed);break;
   case 0xc4:direction_=(direction_&~allowed)|(value&allowed);break;
   case 0xcc:interrupt_level_=(interrupt_level_&~allowed)|(value&allowed);break;
   case 0xd0:{const auto idle=(input_^interrupt_level_)&pins;const auto clear_bits=value&allowed&idle;interrupt_flags_&=~clear_bits;break;}
   case 0xd4:interrupt_mask_=(interrupt_mask_&~allowed)|(value&allowed);break;
   default:return false;
  }
  refresh_interrupts();return true;
 }
 if(off>=0xe0U&&off<=0xfcU){
  if(cpu!=HollywoodCpu::Starlet)return false;
  const auto allowed=(~owner_)&pins;
  switch(off){
   case 0xe0:output_=(output_&~allowed)|(value&allowed);break;
   case 0xe4:direction_=(direction_&~allowed)|(value&allowed);break;
   case 0xec:interrupt_level_=(interrupt_level_&~allowed)|(value&allowed);break;
   case 0xf0:{const auto idle=(~(input_^interrupt_level_))^pins;const auto clear_bits=value&allowed&idle;interrupt_flags_&=~clear_bits;break;}
   case 0xf4:interrupt_mask_=(interrupt_mask_&~allowed)|(value&allowed);break;
   case 0xfc:owner_=value&pins;break;
   default:return false;
  }
  refresh_interrupts();return true;
 }
 return false;
}
void HollywoodGpio::set_inputs(std::uint32_t levels)noexcept{input_=levels&pins;inputs_initialized_=true;refresh_interrupts();}
void HollywoodGpio::refresh_interrupts()noexcept{
 if(!inputs_initialized_)return;
 const auto active=(~(input_^interrupt_level_))&enable_&pins;
 interrupt_flags_|=active;
 if((interrupt_flags_&interrupt_mask_&owner_)!=0U)irq_.raise(10,IrqRoute::Broadway);else irq_.clear(HollywoodCpu::Broadway,bit(10));
 if((interrupt_flags_&interrupt_mask_&~owner_&pins)!=0U)irq_.raise(11,IrqRoute::Starlet);else irq_.clear(HollywoodCpu::Starlet,bit(11));
}
void HollywoodGpio::reset()noexcept{input_=straps_=owner_=output_=direction_=interrupt_level_=interrupt_flags_=interrupt_mask_=0;enable_=pins;inputs_initialized_=false;}

bool HollywoodHardware::CpuBusView::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{return hardware_.read_as(cpu_,off,width,value);}
bool HollywoodHardware::CpuBusView::write(std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{return hardware_.write_as(cpu_,off,width,value);}
bool HollywoodHardware::read(std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{return read_as(HollywoodCpu::Broadway,off,width,value);}
bool HollywoodHardware::write(std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{return write_as(HollywoodCpu::Broadway,off,width,value);}
bool HollywoodHardware::read_as(HollywoodCpu cpu,std::uint32_t off,std::uint8_t width,std::uint64_t& value)noexcept{
 if(width!=4U||(off&3U)!=0U||off>=window_size)return false;
 const auto access_cpu=(cpu==HollywoodCpu::Broadway&&system_control_.ppc_full_access())?HollywoodCpu::Starlet:cpu;
 std::uint32_t v=0;
 if(off<0x10U){const auto ipc_cpu=off==HollywoodIpc::arm_ctrl?access_cpu:cpu;if(!ipc_.read(ipc_cpu,off,v))return false;}
 else if(off==HollywoodTimer::timer_register){if(access_cpu!=HollywoodCpu::Starlet)return false;v=timer_.counter();}
 else if(off==HollywoodTimer::alarm_register){return false;}
 else if(off==HollywoodSystemControl::ahb_prot_register||off==HollywoodSystemControl::clocks_register||off==HollywoodSystemControl::resets_register){if(!system_control_.read(cpu==HollywoodCpu::Broadway&&off!=HollywoodSystemControl::ahb_prot_register?access_cpu:cpu,off,v))return false;}
 else if(off==0x40U){if(access_cpu!=HollywoodCpu::Starlet)return false;v=interrupts_.fiq_mask();}
 else if(off==StarletMemory::sram_protection_register){if(access_cpu!=HollywoodCpu::Starlet)return false;v=starlet_memory_.srnprot();}
 else if(off==StarletMemory::boot0_register){if(access_cpu!=HollywoodCpu::Starlet)return false;v=starlet_memory_.boot0();}
 else if(off>=0x30U&&off<=0x3cU){
  if((off>=0x38U)&&access_cpu!=HollywoodCpu::Starlet)return false;
  switch(off){case 0x30:v=interrupts_.flags(HollywoodCpu::Broadway);break;case 0x34:v=interrupts_.mask(HollywoodCpu::Broadway);break;case 0x38:v=interrupts_.flags(HollywoodCpu::Starlet);break;case 0x3c:v=interrupts_.mask(HollywoodCpu::Starlet);break;default:return false;}
 }
 else if(off>=0xc0U&&off<=0xfcU){if(!gpio_.read(access_cpu,off,v))return false;}
 else return false;
 value=v;return true;
}
bool HollywoodHardware::write_as(HollywoodCpu cpu,std::uint32_t off,std::uint8_t width,std::uint64_t value)noexcept{
 if(width!=4U||(off&3U)!=0U||off>=window_size||value>0xffffffffULL)return false;
 const auto v=static_cast<std::uint32_t>(value);
 const auto access_cpu=(cpu==HollywoodCpu::Broadway&&system_control_.ppc_full_access())?HollywoodCpu::Starlet:cpu;
 if(off<0x10U){const auto ipc_cpu=off==HollywoodIpc::arm_ctrl?access_cpu:cpu;return ipc_.write(ipc_cpu,off,v);}
 if(off==HollywoodTimer::timer_register)return false;
 if(off==HollywoodTimer::alarm_register){if(access_cpu!=HollywoodCpu::Starlet)return false;return timer_.write_alarm(v);}
 if(off==HollywoodSystemControl::ahb_prot_register)return system_control_.write(cpu,off,v);
 if(off==HollywoodSystemControl::clocks_register||off==HollywoodSystemControl::resets_register)return system_control_.write(access_cpu,off,v);
 if(off==0x40U){if(access_cpu!=HollywoodCpu::Starlet)return false;interrupts_.set_fiq_mask(v);return true;}
 if(off==StarletMemory::sram_protection_register){if(access_cpu!=HollywoodCpu::Starlet)return false;starlet_memory_.set_srnprot(v);return true;}
 if(off==StarletMemory::boot0_register){if(access_cpu!=HollywoodCpu::Starlet)return false;starlet_memory_.set_boot0(v);return true;}
 if(off>=0x30U&&off<=0x3cU){
  if((off>=0x38U)&&access_cpu!=HollywoodCpu::Starlet)return false;
  switch(off){case 0x30:interrupts_.clear(HollywoodCpu::Broadway,v);break;case 0x34:interrupts_.set_mask(HollywoodCpu::Broadway,v);break;case 0x38:interrupts_.clear(HollywoodCpu::Starlet,v);break;case 0x3c:interrupts_.set_mask(HollywoodCpu::Starlet,v);break;default:return false;}
  return true;
 }
 if(off>=0xc0U&&off<=0xfcU)return gpio_.write(access_cpu,off,v);
 return false;
}
bool HollywoodHardware::map_into(memory::Memory& broadway_bus,memory::Memory& starlet_bus)noexcept{
 if(!broadway_bus.map_device(base,window_size,broadway_view_))return false;
 return starlet_bus.map_device(mirror,window_size,starlet_view_);
}
bool HollywoodHardware::map_into(WiiMemory& shared_bus)noexcept{
 if(!shared_bus.map_device_dual(base,window_size,broadway_view_,starlet_view_))return false;
 if(!shared_bus.map_device_dual(mirror,window_size,broadway_view_,starlet_view_))return false;
 return starlet_memory_.map_into(shared_bus);
}
void HollywoodHardware::reset()noexcept{interrupts_=HollywoodInterrupts{};ipc_.reset();gpio_.reset();timer_.reset();system_control_.reset();starlet_memory_.reset();}
}
