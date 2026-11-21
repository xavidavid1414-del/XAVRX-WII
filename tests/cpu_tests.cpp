#include "xavrx/cpu/cpu_state.hpp"
#include "xavrx/cpu/decoder.hpp"
#include "xavrx/cpu/executor.hpp"
#include "xavrx/cpu/control_flow.hpp"
#include <cstdint>
#include <iostream>
namespace {
int failures=0;
void check(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';++failures;}}
std::uint32_t d(std::uint8_t op,unsigned rt,unsigned ra,std::uint16_t imm){return(static_cast<std::uint32_t>(op)<<26U)|(rt<<21U)|(ra<<16U)|imm;}
std::uint32_t x(unsigned rt,unsigned ra,unsigned rb,unsigned xo,bool rc=false){return(31U<<26U)|(rt<<21U)|(ra<<16U)|(rb<<11U)|(xo<<1U)|(rc?1U:0U);}
}
int main(){
 using namespace xavrx::cpu;CpuState s;
 check(s.gpr(0)==0U&&s.fpr(31)==0ULL,"zero initialized registers");
 s.set_gpr(31,0xDEADBEEFU);s.set_fpr(30,0x0123456789ABCDEFULL);
 check(s.gpr(31)==0xDEADBEEFU&&s.fpr(30)==0x0123456789ABCDEFULL,"GPR/FPR widths");
 s.set_gpr(32,7U);s.set_fpr(32,9U);check(s.gpr(32)==0U&&s.fpr(32)==0ULL,"safe out-of-range reads");
 s.set_condition_register(0x12345678U);check(s.condition_field(0)==1U&&s.condition_field(7)==8U,"CR field order");
 s.set_condition_field(3,0xAU);check(s.condition_register()==0x123A5678U,"CR field update");
 s.set_condition_field(8,0xFU);check(s.condition_register()==0x123A5678U,"safe CR bounds");
 s.set_pc(0x80003100U);s.set_lr(0x100U);s.set_ctr(4U);s.set_xer(0xE0000000U);s.reset();
 check(s.pc()==0U&&s.lr()==0U&&s.ctr()==0U&&s.xer()==0U&&s.gpr(31)==0U&&s.fpr(30)==0ULL,"reset clears state");
 auto dec=InstructionDecoder::decode(d(14,3,0,0xFFFE));check(dec.id==InstructionId::Addi&&dec.form==InstructionForm::D&&dec.valid(),"D form");
 dec=InstructionDecoder::decode(18U<<26U);check(dec.id==InstructionId::Branch&&dec.form==InstructionForm::I,"I form");
 dec=InstructionDecoder::decode(x(3,4,5,266));check(dec.id==InstructionId::Add&&dec.has_extended_opcode&&dec.extended_opcode==266,"extended opcode");
 check(InstructionDecoder::decode(17U<<26U).form==InstructionForm::SC,"system-call form");
 check(InstructionDecoder::decode((19U<<26U)|(16U<<1U)).id==InstructionId::BranchLinkRegister,"XL form");
 check(InstructionDecoder::decode((63U<<26U)|(21U<<1U)).id==InstructionId::FloatAdd,"A form");
 check(InstructionDecoder::decode(d(3,0,0,0)).id==InstructionId::TrapImmediate,"twi decode");
 check(InstructionDecoder::decode(x(0,0,0,4)).id==InstructionId::TrapWord,"tw decode");
 check(!InstructionDecoder::decode((63U<<26U)|(999U<<1U)).valid()&&!InstructionDecoder::decode(0xFFFFFFFFU).valid(),"unknown encodings rejected");
 s.reset();s.set_pc(0x1000U);
 check(IntegerExecutor::execute(d(14,3,0,0xFFFE),s)==ExecutionResult::Executed&&s.gpr(3)==0xFFFFFFFEU&&s.pc()==0x1004U,"addi sign extends and uses zero base");
 s.set_gpr(4,5U);check(IntegerExecutor::execute(d(15,3,4,2),s)==ExecutionResult::Executed&&s.gpr(3)==0x00020005U,"addis shifts signed immediate");
 s.set_gpr(5,0x12340000U);
 check(IntegerExecutor::execute(d(24,5,6,0x00AB),s)==ExecutionResult::Executed&&s.gpr(6)==0x123400ABU,"ori");
 check(IntegerExecutor::execute(d(25,5,7,0x00AB),s)==ExecutionResult::Executed&&s.gpr(7)==0x12BF0000U,"oris");
 check(IntegerExecutor::execute(d(26,5,8,0xFFFF),s)==ExecutionResult::Executed&&s.gpr(8)==0x1234FFFFU,"xori");
 check(IntegerExecutor::execute(d(27,5,9,0x00FF),s)==ExecutionResult::Executed&&s.gpr(9)==0x12CB0000U,"xoris");
 check(IntegerExecutor::execute(d(28,5,10,0x00FF),s)==ExecutionResult::Executed&&s.gpr(10)==0U&&s.condition_field(0)==2U,"andi. and CR0 equal");
 s.set_gpr(4,7U);s.set_gpr(5,9U);
 check(IntegerExecutor::execute(x(3,4,5,266),s)==ExecutionResult::Executed&&s.gpr(3)==16U,"add");
 check(IntegerExecutor::execute(x(3,4,5,40),s)==ExecutionResult::Executed&&s.gpr(3)==2U,"subf computes RB minus RA");
 check(IntegerExecutor::execute(x(4,3,5,28),s)==ExecutionResult::Executed&&s.gpr(3)==1U,"and writes RA destination");
 check(IntegerExecutor::execute(x(4,3,5,444),s)==ExecutionResult::Executed&&s.gpr(3)==15U,"or writes RA destination");
 check(IntegerExecutor::execute(x(4,3,5,316),s)==ExecutionResult::Executed&&s.gpr(3)==14U,"xor writes RA destination");
 s.set_gpr(4,0xFFFFFFFFU);s.set_gpr(5,1U);s.set_xer(0x80000000U);
 check(IntegerExecutor::execute(x(3,4,5,266,true),s)==ExecutionResult::Executed&&s.gpr(3)==0U&&s.condition_field(0)==3U,"add record sets EQ and SO");
 const auto cmp=(31U<<26U)|(2U<<23U)|(4U<<16U)|(5U<<11U);s.set_gpr(4,0xFFFFFFFFU);s.set_gpr(5,1U);
 check(IntegerExecutor::execute(cmp,s)==ExecutionResult::Executed&&s.condition_field(2)==9U,"signed compare writes selected CR field and SO");
 const auto oldpc=s.pc(),oldgpr=s.gpr(3);
 check(IntegerExecutor::execute(x(3,4,5,266)|(1U<<10U),s)==ExecutionResult::UnsupportedEncoding&&s.pc()==oldpc&&s.gpr(3)==oldgpr,"OE encoding rejected without state change");
 check(IntegerExecutor::execute(0xFFFFFFFFU,s)==ExecutionResult::InvalidInstruction&&s.pc()==oldpc,"invalid instruction leaves state unchanged");
 // B, AA, LK and signed displacement.
 s.reset();s.set_pc(0x1000U);
 check(ControlFlowExecutor::execute((18U<<26U)|(0x10U<<2U)|1U,s)==ControlFlowResult::Executed&&s.pc()==0x1040U&&s.lr()==0x1004U,"relative branch with link");
 s.set_pc(0x2000U);
 check(ControlFlowExecutor::execute((18U<<26U)|(0xFFFFFFU<<2U),s)==ControlFlowResult::Executed&&s.pc()==0x1FFCU,"negative branch displacement");
 s.set_pc(0x3000U);
 check(ControlFlowExecutor::execute((18U<<26U)|(0x20U<<2U)|2U,s)==ControlFlowResult::Executed&&s.pc()==0x80U,"absolute branch");
 // BC tests CR and CTR.
 s.reset();s.set_pc(0x4000U);s.set_condition_register(0x80000000U);
 const auto bc=(16U<<26U)|(12U<<21U)|(4U<<2U);
 check(ControlFlowExecutor::execute(bc,s)==ControlFlowResult::Executed&&s.pc()==0x4010U,"conditional branch taken");
 s.set_pc(0x4000U);s.set_condition_register(0U);
 check(ControlFlowExecutor::execute(bc,s)==ControlFlowResult::Executed&&s.pc()==0x4004U,"conditional branch not taken");
 s.set_pc(0x5000U);s.set_ctr(2U);const auto bdnz=(16U<<26U)|(16U<<21U)|(8U<<2U);
 check(ControlFlowExecutor::execute(bdnz,s)==ControlFlowResult::Executed&&s.pc()==0x5020U&&s.ctr()==1U,"bdnz decrements and branches");
 s.set_pc(0x5000U);s.set_ctr(1U);
 check(ControlFlowExecutor::execute(bdnz,s)==ControlFlowResult::Executed&&s.pc()==0x5004U&&s.ctr()==0U,"bdnz falls through at zero");
 s.set_pc(0x6000U);s.set_lr(0x7777U);const auto blr=(19U<<26U)|(20U<<21U)|(16U<<1U);
 check(ControlFlowExecutor::execute(blr,s)==ControlFlowResult::Executed&&s.pc()==0x7774U,"blr uses old aligned LR target");
 s.set_pc(0x7000U);s.set_ctr(0x888BU);const auto bctr=(19U<<26U)|(20U<<21U)|(528U<<1U);
 check(ControlFlowExecutor::execute(bctr,s)==ControlFlowResult::Executed&&s.pc()==0x8888U,"bctr uses aligned CTR target");
 // Exceptions, trap instructions and RFI.
 s.reset();s.set_pc(0x9000U);s.set_msr(0xC000U);
 check(ControlFlowExecutor::execute(17U<<26U,s)==ControlFlowResult::Executed&&s.pc()==0xC00U&&s.srr0()==0x9000U&&s.srr1()==0xC000U&&s.msr()==0U,"system call saves state");
 check(ControlFlowExecutor::return_from_interrupt(s)==ControlFlowResult::Executed&&s.pc()==0x9000U&&s.msr()==0xC000U,"rfi restores state");
 s.set_pc(0xA000U);s.set_msr(0x8000U);
 check(ControlFlowExecutor::raise_exception(ExceptionKind::ExternalInterrupt,s)==ControlFlowResult::Executed&&s.pc()==0x500U&&s.srr0()==0xA000U,"enabled external interrupt");
 check(ControlFlowExecutor::raise_exception(ExceptionKind::ExternalInterrupt,s)==ControlFlowResult::InterruptMasked&&s.pc()==0x500U,"masked external interrupt");
 s.set_pc(0xB000U);
 check(ControlFlowExecutor::raise_exception(ExceptionKind::Program,s)==ControlFlowResult::Executed&&s.pc()==0x700U&&s.srr0()==0xB000U,"program exception vector");
 s.reset();s.set_pc(0xC000U);s.set_gpr(4,9U);
 const auto twi=(3U<<26U)|(4U<<21U)|(4U<<16U)|5U;
 check(ControlFlowExecutor::execute(twi,s)==ControlFlowResult::Executed&&s.pc()==0x700U&&s.srr0()==0xC000U,"twi raises program exception on signed greater");
 s.reset();s.set_pc(0xD000U);s.set_gpr(4,3U);s.set_gpr(5,3U);
 const auto tw=(31U<<26U)|(4U<<21U)|(4U<<16U)|(5U<<11U)|(4U<<1U);
 check(ControlFlowExecutor::execute(tw,s)==ControlFlowResult::Executed&&s.pc()==0x700U,"tw raises program exception on equality");
 s.reset();s.set_pc(0xE000U);s.set_gpr(4,1U);
 check(ControlFlowExecutor::execute((3U<<26U)|(4U<<21U)|(4U<<16U)|5U,s)==ControlFlowResult::Executed&&s.pc()==0xE004U,"twi continues when trap condition false");
 if(failures){std::cerr<<failures<<" CPU test(s) failed\n";return 1;}
 std::cout<<"All XAVRX-WII CPU, integer, control-flow and exception tests passed\n";return 0;
}
