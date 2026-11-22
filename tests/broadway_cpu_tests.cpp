#include "xavrx/core/emulator.hpp"
#include "xavrx/core/broadway_cpu.hpp"
#include "xavrx/cpu/memory_access.hpp"
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}
std::uint32_t d(unsigned op,unsigned rt,unsigned ra,std::uint16_t imm){return(static_cast<std::uint32_t>(op)<<26U)|(rt<<21U)|(ra<<16U)|imm;}}
int main(){
 using namespace xavrx;Emulator emu;check(emu.initialize(),"emulator initializes before Broadway execution");
 auto* sys=emu.system();if(!sys){std::cerr<<"missing WiiSystem\n";return 1;}
 auto& cpu=emu.broadway_cpu();cpu.state().set_pc(system::WiiMemory::mem1_cached);
 check(sys->memory().write32(system::WiiMemory::mem1_cached,d(14,3,0,42U))==memory::AccessResult::Success,"write ADDI instruction");
 check(emu.step_broadway()==core::CpuStepResult::Executed&&cpu.state().gpr(3)==42U&&cpu.state().pc()==system::WiiMemory::mem1_cached+4U,"Broadway fetches and executes ADDI from Wii memory");
 check(sys->scheduled_cycles(system::ClockDomain::Cpu)==1U,"Broadway instruction advances system clock scheduler");
 cpu.state().set_gpr(4,system::WiiMemory::mem1_cached+0x100U);cpu.state().set_gpr(5,0xdecafbadU);
 const auto stw=d(36,5,4,0U),lwz=d(32,6,4,0U);
 check(cpu::LoadStoreExecutor::execute(stw,cpu.state(),sys->memory())==cpu::MemoryExecutionResult::Executed,"STW writes a big-endian Wii memory word");
 check(cpu::LoadStoreExecutor::execute(lwz,cpu.state(),sys->memory())==cpu::MemoryExecutionResult::Executed&&cpu.state().gpr(6)==0xdecafbadU,"LWZ reads the stored Wii memory word");
 const auto oldpc=cpu.state().pc();check(cpu::LoadStoreExecutor::execute(lwz,cpu.state(),sys->memory())==cpu::MemoryExecutionResult::Executed&&cpu.state().pc()==oldpc+4U,"load-store executor advances PC only after success");
 cpu.reset();check(cpu.state().pc()==0U&&cpu.state().gpr(3)==0U,"Broadway CPU reset clears architectural state");
 if(failures){std::cerr<<failures<<" Broadway CPU test(s) failed\n";return 1;}
 std::cout<<"All Broadway CPU integration tests passed\n";return 0;
}
