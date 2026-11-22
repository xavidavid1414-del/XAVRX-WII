#include "xavrx/core/broadway_cpu.hpp"
#include "xavrx/cpu/control_flow.hpp"
#include "xavrx/cpu/decoder.hpp"
#include "xavrx/cpu/executor.hpp"
#include "xavrx/cpu/memory_access.hpp"
#include "xavrx/system/wii_system.hpp"
namespace xavrx::core {
CpuStepResult BroadwayCpu::step(system::WiiSystem& system) noexcept {
 if(!system.initialized())return CpuStepResult::NotInitialized;
 if(system.processor_interrupts().active()!=0U){
  const auto irq=cpu::ControlFlowExecutor::raise_exception(cpu::ExceptionKind::ExternalInterrupt,state_);
  if(irq==cpu::ControlFlowResult::Executed){system.advance_broadway_cycles(1U);return CpuStepResult::InterruptTaken;}
 }
 std::uint64_t fetched=0;
 if(system.memory().read_as(system::WiiMemoryCpu::Broadway,state_.pc(),4U,fetched)!=xavrx::memory::AccessResult::Success)return CpuStepResult::BusFault;
 const auto word=static_cast<std::uint32_t>(fetched);
 const auto decoded=cpu::InstructionDecoder::decode(word);
 if(!decoded.valid())return CpuStepResult::InvalidInstruction;
 if(decoded.id==cpu::InstructionId::LoadWord||decoded.id==cpu::InstructionId::StoreWord){
  const auto result=cpu::LoadStoreExecutor::execute(word,state_,system.memory());
  if(result==cpu::MemoryExecutionResult::BusFault)return CpuStepResult::BusFault;
  if(result!=cpu::MemoryExecutionResult::Executed)return CpuStepResult::UnsupportedInstruction;
 }else if(decoded.id==cpu::InstructionId::Branch||decoded.id==cpu::InstructionId::BranchConditional||
          decoded.id==cpu::InstructionId::BranchLinkRegister||decoded.id==cpu::InstructionId::BranchCountRegister||
          decoded.id==cpu::InstructionId::ReturnFromInterrupt||decoded.id==cpu::InstructionId::SystemCall||
          decoded.id==cpu::InstructionId::TrapImmediate||decoded.id==cpu::InstructionId::TrapWord){
  const auto result=cpu::ControlFlowExecutor::execute(word,state_);
  if(result==cpu::ControlFlowResult::InvalidInstruction)return CpuStepResult::InvalidInstruction;
  if(result!=cpu::ControlFlowResult::Executed)return CpuStepResult::UnsupportedInstruction;
 }else{
  const auto result=cpu::IntegerExecutor::execute(word,state_);
  if(result==cpu::ExecutionResult::InvalidInstruction)return CpuStepResult::InvalidInstruction;
  if(result!=cpu::ExecutionResult::Executed)return CpuStepResult::UnsupportedInstruction;
 }
 system.advance_broadway_cycles(1U);
 return CpuStepResult::Executed;
}
}
