#include "xavrx/system/wii_memory.hpp"
#include "xavrx/memory/memory.hpp"
#include <array>
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
class TestDevice final:public xavrx::memory::MemoryMappedDevice {
public:
 bool read(std::uint32_t,std::uint8_t width,std::uint64_t& value)noexcept override{if(width!=4U)return false;value=0x5a5aa5a5U;return true;}
 bool write(std::uint32_t,std::uint8_t width,std::uint64_t)noexcept override{return width==4U;}
};
int main(){using namespace xavrx::system;using xavrx::memory::AccessResult;
 WiiMemory memory;
 const auto invalid_cpu=static_cast<WiiMemoryCpu>(0xffU);
 std::uint64_t invalid_probe=0xfeedfaceU;
 check(memory.read_as(invalid_cpu,WiiMemory::mem1_cached,4U,invalid_probe)==AccessResult::InvalidConfiguration&&invalid_probe==0xfeedfaceU,"invalid CPU identity is rejected without changing read output");
 check(memory.write_as(invalid_cpu,WiiMemory::mem1_cached,4U,0x12345678U)==AccessResult::InvalidConfiguration,"invalid CPU identity is rejected before memory writes");
 TestDevice existing_device,fresh_device;
 check(memory.map_device_dual(0x20000000U,0x1000U,existing_device,existing_device),"register initial MMIO device");
 const std::array<DualDeviceMapping,2> conflicting_batch{{
  {0x30000000U,0x1000U,&fresh_device,&fresh_device},
  {0x20000000U,0x1000U,&fresh_device,&fresh_device}
 }};
 check(!memory.map_devices_dual(conflicting_batch.data(),conflicting_batch.size()),"conflicting MMIO batch rejected");
 std::uint64_t probe=0;
 check(memory.read_as(WiiMemoryCpu::Starlet,0x30000000U,4,probe)==AccessResult::Unmapped,"failed MMIO batch leaves no partial registration");
 check(memory.read_as(WiiMemoryCpu::Starlet,0x20000000U,4,probe)==AccessResult::Success&&probe==0x5a5aa5a5U,"pre-existing MMIO registration survives failed batch");
 check(memory.mem1_capacity()==24U*1024U*1024U,"MEM1 capacity is 24 MiB");check(memory.mem2_capacity()==64U*1024U*1024U,"MEM2 capacity is 64 MiB");
 check(memory.write32(WiiMemory::mem1_cached,0x12345678U)==AccessResult::Success,"write MEM1 cached alias");std::uint32_t value=0;check(memory.read32(WiiMemory::mem1_uncached,value)==AccessResult::Success&&value==0x12345678U,"MEM1 cached and uncached aliases share backing");check(memory.read32(WiiMemory::mem1_physical,value)==AccessResult::Success&&value==0x12345678U,"MEM1 physical alias shares backing");
 check(memory.write64(WiiMemory::mem2_cached,0x0123456789ABCDEFULL)==AccessResult::Success,"write MEM2 cached alias");std::uint64_t wide=0;check(memory.read64(WiiMemory::mem2_uncached,wide)==AccessResult::Success&&wide==0x0123456789ABCDEFULL,"MEM2 uncached alias shares backing");check(memory.read64(WiiMemory::mem2_physical,wide)==AccessResult::Success&&wide==0x0123456789ABCDEFULL,"MEM2 physical alias shares backing");
 check(memory.write32(WiiMemory::mem1_cached+static_cast<std::uint32_t>(WiiMemory::mem1_size)-4U,0xAABBCCDDU)==AccessResult::Success,"last MEM1 word accessible");check(memory.read32(WiiMemory::mem1_uncached+static_cast<std::uint32_t>(WiiMemory::mem1_size)-4U,value)==AccessResult::Success&&value==0xAABBCCDDU,"MEM1 upper boundary alias");check(memory.read32(WiiMemory::mem1_cached+static_cast<std::uint32_t>(WiiMemory::mem1_size),value)==AccessResult::Unmapped,"MEM1 boundary rejected");
 check(memory.read32(WiiMemory::mem2_cached+static_cast<std::uint32_t>(WiiMemory::mem2_size),value)==AccessResult::Unmapped,"MEM2 boundary rejected");check(memory.read32(WiiMemory::mem1_cached+1U,value)==AccessResult::Misaligned,"misaligned access rejected");check(memory.read(WiiMemory::mem1_cached,3,wide)==AccessResult::InvalidWidth,"invalid width rejected");check(memory.read32(0x70000000U,value)==AccessResult::Unmapped,"unmapped address rejected");
 memory.reset();check(memory.read32(WiiMemory::mem1_cached,value)==AccessResult::Success&&value==0U,"reset clears MEM1");check(memory.read64(WiiMemory::mem2_cached,wide)==AccessResult::Success&&wide==0U,"reset clears MEM2");
 if(failures){std::cerr<<failures<<" Wii memory test(s) failed\n";return 1;}std::cout<<"All Wii memory map tests passed\n";return 0;}
