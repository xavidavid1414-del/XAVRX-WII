#include "xavrx/cpu/executor.hpp"
#include "xavrx/cpu/decoder.hpp"
#include <bit>
#include <cstdint>
namespace xavrx::cpu {
namespace {
constexpr std::uint32_t field(std::uint32_t w,unsigned shift) noexcept{return(w>>shift)&0x1FU;}
constexpr std::uint16_t imm16(std::uint32_t w) noexcept{return static_cast<std::uint16_t>(w&0xFFFFU);}
constexpr std::int32_t signed_imm16(std::uint32_t w) noexcept{
 const auto value=static_cast<std::uint32_t>(imm16(w));
 return value<0x8000U?static_cast<std::int32_t>(value):static_cast<std::int32_t>(value)-65536;
}
void set_cr0(CpuState& s,std::uint32_t value) noexcept{
 const auto signed_value=std::bit_cast<std::int32_t>(value);
 const std::uint8_t flags=static_cast<std::uint8_t>((signed_value<0?8U:0U)|(signed_value>0?4U:0U)|(signed_value==0?2U:0U)|((s.xer()>>31U)&1U));
 s.set_condition_field(0,flags);
}
bool record(std::uint32_t w) noexcept{return(w&1U)!=0U;}
}
ExecutionResult IntegerExecutor::execute(std::uint32_t word,CpuState& s) noexcept{
 const auto decoded=InstructionDecoder::decode(word);
 if(!decoded.valid())return ExecutionResult::InvalidInstruction;
 const auto rt=field(word,21U),ra=field(word,16U),rb=field(word,11U),rs=field(word,21U);
 std::uint32_t result=0U; bool write_result=false,update_cr=false;
 switch(decoded.id){
 case InstructionId::Addi:
   result=(ra==0U?0U:s.gpr(ra))+static_cast<std::uint32_t>(signed_imm16(word));write_result=true;break;
 case InstructionId::Addis:
   result=(ra==0U?0U:s.gpr(ra))+(static_cast<std::uint32_t>(signed_imm16(word))<<16U);write_result=true;break;
 case InstructionId::Ori:
   result=s.gpr(rs)|static_cast<std::uint32_t>(imm16(word));write_result=true;break;
 case InstructionId::Oris:
   result=s.gpr(rs)|(static_cast<std::uint32_t>(imm16(word))<<16U);write_result=true;break;
 case InstructionId::Xori:
   result=s.gpr(rs)^static_cast<std::uint32_t>(imm16(word));write_result=true;break;
 case InstructionId::Xoris:
   result=s.gpr(rs)^(static_cast<std::uint32_t>(imm16(word))<<16U);write_result=true;break;
 case InstructionId::AndiDot:
   result=s.gpr(rs)&static_cast<std::uint32_t>(imm16(word));write_result=true;update_cr=true;break;
 case InstructionId::Add:
   if((word&(1U<<10U))!=0U)return ExecutionResult::UnsupportedEncoding;
   result=s.gpr(ra)+s.gpr(rb);write_result=true;update_cr=record(word);break;
 case InstructionId::Subf:
   if((word&(1U<<10U))!=0U)return ExecutionResult::UnsupportedEncoding;
   result=s.gpr(rb)-s.gpr(ra);write_result=true;update_cr=record(word);break;
 case InstructionId::And:
   result=s.gpr(rs)&s.gpr(rb);write_result=true;update_cr=record(word);break;
 case InstructionId::Or:
   result=s.gpr(rs)|s.gpr(rb);write_result=true;update_cr=record(word);break;
 case InstructionId::Xor:
   result=s.gpr(rs)^s.gpr(rb);write_result=true;update_cr=record(word);break;
 case InstructionId::Compare:{
   if((word&(1U<<21U))!=0U)return ExecutionResult::UnsupportedEncoding;
   const auto a=std::bit_cast<std::int32_t>(s.gpr(ra));
   const auto b=std::bit_cast<std::int32_t>(s.gpr(rb));
   const std::uint8_t flags=static_cast<std::uint8_t>((a<b?8U:0U)|(a>b?4U:0U)|(a==b?2U:0U)|((s.xer()>>31U)&1U));
   const auto bf=field(word,23U);
   s.set_condition_field(bf,flags);break;
 }
 default:return ExecutionResult::UnsupportedEncoding;
 }
 if(write_result)s.set_gpr(decoded.id==InstructionId::Ori||decoded.id==InstructionId::Oris||decoded.id==InstructionId::Xori||decoded.id==InstructionId::Xoris||decoded.id==InstructionId::AndiDot?ra:rt,result);
 if(update_cr)set_cr0(s,result);
 s.set_pc(s.pc()+4U);
 return ExecutionResult::Executed;
}
}
