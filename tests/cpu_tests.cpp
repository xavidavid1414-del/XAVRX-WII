#include "xavrx/cpu/cpu_state.hpp"
#include "xavrx/cpu/decoder.hpp"
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* msg){if(!ok){std::cerr<<"FAIL: "<<msg<<'\n';++failures;}}
std::uint32_t enc(std::uint8_t op,std::uint16_t xo=0U){return(static_cast<std::uint32_t>(op)<<26U)|(static_cast<std::uint32_t>(xo)<<1U);}}
int main(){using namespace xavrx::cpu;CpuState s;
check(s.gpr(0)==0U&&s.fpr(31)==0ULL,"zero initialized registers");
s.set_gpr(31,0xDEADBEEFU);s.set_fpr(30,0x0123456789ABCDEFULL);
check(s.gpr(31)==0xDEADBEEFU&&s.fpr(30)==0x0123456789ABCDEFULL,"GPR/FPR widths");
s.set_gpr(32,7U);s.set_fpr(32,9U);check(s.gpr(32)==0U&&s.fpr(32)==0ULL,"safe out-of-range reads");
s.set_condition_register(0x12345678U);check(s.condition_field(0)==1U&&s.condition_field(7)==8U,"CR field order");
s.set_condition_field(3,0xAU);check(s.condition_register()==0x123A5678U,"CR field update");
s.set_condition_field(8,0xFU);check(s.condition_register()==0x123A5678U,"safe CR bounds");
s.set_pc(0x80003100U);s.set_lr(0x100U);s.set_ctr(4U);s.set_xer(0xE0000000U);s.reset();
check(s.pc()==0U&&s.lr()==0U&&s.ctr()==0U&&s.xer()==0U&&s.gpr(31)==0U&&s.fpr(30)==0ULL,"reset clears state");
auto d=InstructionDecoder::decode(enc(14));check(d.id==InstructionId::Addi&&d.form==InstructionForm::D&&d.valid(),"D form");
d=InstructionDecoder::decode(enc(18));check(d.id==InstructionId::Branch&&d.form==InstructionForm::I,"I form");
d=InstructionDecoder::decode(enc(31,266));check(d.id==InstructionId::Add&&d.has_extended_opcode&&d.extended_opcode==266,"extended opcode");
check(InstructionDecoder::decode(enc(17)).form==InstructionForm::SC,"system-call form");
check(InstructionDecoder::decode(enc(19,16)).id==InstructionId::BranchLinkRegister,"XL form");
check(InstructionDecoder::decode(enc(63,21)).id==InstructionId::FloatAdd,"A form");
check(!InstructionDecoder::decode(enc(63,999)).valid()&&!InstructionDecoder::decode(0xFFFFFFFFU).valid(),"unknown encodings rejected");
if(failures)return 1;std::cout<<"All XAVRX-WII CPU foundation tests passed\n";return 0;}