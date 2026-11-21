#include "xavrx/cpu/control_flow.hpp"
#include "xavrx/cpu/decoder.hpp"
#include <bit>
#include <cstdint>
namespace xavrx::cpu {
namespace {
constexpr std::uint32_t field(std::uint32_t word,unsigned shift,unsigned width) noexcept{return(word>>shift)&((1U<<width)-1U);}
constexpr std::int32_t sign_extend(std::uint32_t value,unsigned bits) noexcept{
 const auto sign=1U<<(bits-1U),mask=(1U<<bits)-1U;value&=mask;
 return static_cast<std::int32_t>(value^sign)-static_cast<std::int32_t>(sign);
}
constexpr std::uint32_t target(std::uint32_t pc,std::int32_t displacement,bool absolute) noexcept{
 return((absolute?0U:pc)+static_cast<std::uint32_t>(displacement))&~3U;
}
bool conditional_taken(std::uint32_t word,CpuState& state) noexcept{
 const auto bo=field(word,21U,5U),bi=field(word,16U,5U);bool ctr_ok=true,cr_ok=true;
 if((bo&0x04U)==0U){state.set_ctr(state.ctr()-1U);ctr_ok=(state.ctr()!=0U)!=((bo&0x02U)!=0U);}
 if((bo&0x10U)==0U){const auto bit=(state.condition_register()>>(31U-bi))&1U;cr_ok=bit==static_cast<std::uint32_t>((bo&0x08U)!=0U);}
 return ctr_ok&&cr_ok;
}
bool trap_matches(std::uint32_t options,std::uint32_t lhs,std::uint32_t rhs) noexcept{
 const auto sl=std::bit_cast<std::int32_t>(lhs),sr=std::bit_cast<std::int32_t>(rhs);
 return(((options&0x10U)!=0U)&&(sl<sr))||(((options&0x08U)!=0U)&&(sl>sr))||
       (((options&0x04U)!=0U)&&(lhs==rhs))||(((options&0x02U)!=0U)&&(lhs<rhs))||
       (((options&0x01U)!=0U)&&(lhs>rhs));
}
void enter_exception(CpuState& state,std::uint32_t vector) noexcept{
 state.set_srr0(state.pc());state.set_srr1(state.msr());
 constexpr std::uint32_t msr_ee=1U<<15U,msr_pr=1U<<14U;
 state.set_msr(state.msr()&~(msr_ee|msr_pr));state.set_pc(vector);
}
}
ControlFlowResult ControlFlowExecutor::execute(std::uint32_t word,CpuState& state) noexcept{
 const auto decoded=InstructionDecoder::decode(word);if(!decoded.valid())return ControlFlowResult::InvalidInstruction;
 const auto pc=state.pc();const bool lk=(word&1U)!=0U;
 switch(decoded.id){
 case InstructionId::Branch:{
  const auto disp=sign_extend((word>>2U)&0x00FFFFFFU,24U)*4;
  if(lk)state.set_lr(pc+4U);state.set_pc(target(pc,disp,(word&2U)!=0U));return ControlFlowResult::Executed;
 }
 case InstructionId::BranchConditional:{
  const auto disp=sign_extend((word>>2U)&0x3FFFU,14U)*4;const bool taken=conditional_taken(word,state);
  if(lk)state.set_lr(pc+4U);state.set_pc(taken?target(pc,disp,(word&2U)!=0U):pc+4U);return ControlFlowResult::Executed;
 }
 case InstructionId::BranchLinkRegister:{
  const auto destination=state.lr()&~3U;const bool taken=conditional_taken(word,state);
  if(lk)state.set_lr(pc+4U);state.set_pc(taken?destination:pc+4U);return ControlFlowResult::Executed;
 }
 case InstructionId::BranchCountRegister:{
  const auto bo=field(word,21U,5U);if((bo&0x04U)==0U)return ControlFlowResult::UnsupportedEncoding;
  const auto destination=state.ctr()&~3U;const auto bi=field(word,16U,5U);
  const bool cr_ok=(bo&0x10U)!=0U||(((state.condition_register()>>(31U-bi))&1U)==static_cast<std::uint32_t>((bo&0x08U)!=0U));
  if(lk)state.set_lr(pc+4U);state.set_pc(cr_ok?destination:pc+4U);return ControlFlowResult::Executed;
 }
 case InstructionId::SystemCall:return raise_exception(ExceptionKind::SystemCall,state);
 case InstructionId::ReturnFromInterrupt:return return_from_interrupt(state);
 case InstructionId::TrapImmediate:{
  const auto options=field(word,21U,5U),ra=field(word,16U,5U);
  const auto rhs=static_cast<std::uint32_t>(sign_extend(word&0xFFFFU,16U));
  if(trap_matches(options,state.gpr(ra),rhs))return raise_exception(ExceptionKind::Program,state);
  state.set_pc(pc+4U);return ControlFlowResult::Executed;
 }
 case InstructionId::TrapWord:{
  const auto options=field(word,21U,5U),ra=field(word,16U,5U),rb=field(word,11U,5U);
  if(trap_matches(options,state.gpr(ra),state.gpr(rb)))return raise_exception(ExceptionKind::Program,state);
  state.set_pc(pc+4U);return ControlFlowResult::Executed;
 }
 default:return ControlFlowResult::UnsupportedEncoding;
 }
}
ControlFlowResult ControlFlowExecutor::raise_exception(ExceptionKind kind,CpuState& state) noexcept{
 std::uint32_t vector=0U;
 switch(kind){
 case ExceptionKind::SystemCall:vector=0xC00U;break;
 case ExceptionKind::Program:vector=0x700U;break;
 case ExceptionKind::ExternalInterrupt:
  if((state.msr()&(1U<<15U))==0U)return ControlFlowResult::InterruptMasked;
  vector=0x500U;break;
 }
 enter_exception(state,vector);return ControlFlowResult::Executed;
}
ControlFlowResult ControlFlowExecutor::return_from_interrupt(CpuState& state) noexcept{
 state.set_pc(state.srr0());state.set_msr(state.srr1());return ControlFlowResult::Executed;
}
}
