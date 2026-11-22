#include "xavrx/cpu/memory_access.hpp"
#include "xavrx/cpu/decoder.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <cstdint>
namespace xavrx::cpu {
MemoryExecutionResult LoadStoreExecutor::execute(std::uint32_t word,CpuState& state,system::WiiMemory& memory) noexcept {
 const auto decoded=InstructionDecoder::decode(word);
 if(!decoded.valid())return MemoryExecutionResult::InvalidInstruction;
 if(decoded.id!=InstructionId::LoadWord&&decoded.id!=InstructionId::StoreWord)return MemoryExecutionResult::UnsupportedEncoding;
 const auto reg=(word>>21U)&31U,ra=(word>>16U)&31U;
 const auto raw=static_cast<std::uint16_t>(word&0xffffU);
 const auto displacement=raw<0x8000U?static_cast<std::int32_t>(raw):static_cast<std::int32_t>(raw)-65536;
 const auto base=ra==0U?0U:state.gpr(ra);
 const auto address=base+static_cast<std::uint32_t>(displacement);
 if(decoded.id==InstructionId::LoadWord){
  std::uint64_t loaded=0;
  if(memory.read_as(system::WiiMemoryCpu::Broadway,address,4U,loaded)!=xavrx::memory::AccessResult::Success)return MemoryExecutionResult::BusFault;
  state.set_gpr(reg,static_cast<std::uint32_t>(loaded));
 }else{
  if(memory.write_as(system::WiiMemoryCpu::Broadway,address,4U,state.gpr(reg))!=xavrx::memory::AccessResult::Success)return MemoryExecutionResult::BusFault;
 }
 state.set_pc(state.pc()+4U);return MemoryExecutionResult::Executed;
}
}
