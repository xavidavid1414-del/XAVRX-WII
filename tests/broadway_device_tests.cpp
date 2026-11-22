#include "xavrx/system/broadway_devices.hpp"
#include <cstdint>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char* m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}}
int main(){
 using namespace xavrx::system;std::uint64_t value=0;std::uint32_t mail=0;
 DspMailboxInterface dsp;
 check(dsp.write(0x00U,2,0x9234U)&&dsp.write(0x02U,2,0x5678U),"DSP inbound mailbox accepts high and low halfwords");
 check(dsp.take_mail_to_dsp(mail)&&mail==0x12345678U,"DSP consumes one complete inbound mailbox message");
 check(!dsp.take_mail_to_dsp(mail),"DSP inbound mailbox clears pending after consumption");
 check(dsp.post_mail_from_dsp(0x89abcdefU),"DSP posts outbound mailbox message");
 check(dsp.read(0x04U,2,value)&&value==0x89abU+0x8000U,"DSP outbound high halfword marks message valid");
 check(dsp.read(0x06U,2,value)&&value==0xcdefU,"DSP outbound low halfword returns payload");
 check(!dsp.read(0x00U,2,value)&&!dsp.write(0x04U,2,0U),"DSP mailbox rejects unsupported access direction");
 dsp.reset();check(!dsp.read(0x04U,2,value)||value==0U,"DSP mailbox reset clears output state");
 ProcessorInterruptInterface pi;
 pi.raise(6U);pi.raise(11U);check(pi.cause()==((1U<<6U)|(1U<<11U)),"PI latches DSP and GX FIFO interrupt causes");
 check(pi.write(4U,4,(1U<<6U)|(1U<<11U))&&pi.active()==pi.cause(),"PI mask gates pending causes");
 check(pi.write(0U,4,1U<<6U)&&pi.cause()==(1U<<11U),"PI cause register is write-one-to-clear");
 check(!pi.write(0U,2,1U),"PI rejects non-word register access");
 GpuCommandProcessor gpu;GxWriteGatherPort pipe(gpu);
 check(pipe.write(0U,4,0x11223344U)&&pipe.write(0U,4,0xaabbccddU),"GX write-gather port queues 32-bit words");
 std::uint32_t word=0;check(gpu.take(word)&&word==0x11223344U&&gpu.take(word)&&word==0xaabbccddU,"GX FIFO preserves write order");
 check(!pipe.write(0U,2,0U)&&!pipe.write(4U,4,0U),"GX FIFO rejects invalid width and offset");
 if(failures){std::cerr<<failures<<" Broadway device test(s) failed\n";return 1;}
 std::cout<<"All Broadway DSP, PI and GX FIFO tests passed\n";return 0;
}
