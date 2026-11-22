#include "xavrx/system/hardware.hpp"
#include "xavrx/system/wii_system.hpp"
#include "xavrx/system/reset_controller.hpp"
#include "xavrx/system/dsp_gpu.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
namespace {int failures=0;void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';++failures;}}}
int main(){using namespace xavrx::system;
 SystemClock clock;check(clock.valid(),"default clock valid");check(clock.frequency(ClockDomain::Cpu)==729000000ULL,"CPU clock frequency");check(clock.frequency(ClockDomain::Gpu)==243000000ULL,"GPU clock frequency");check(clock.frequency(ClockDomain::Bus)==162000000ULL,"bus clock frequency");check(clock.frequency(ClockDomain::Dsp)==121500000ULL,"DSP clock frequency");check(clock.frequency(ClockDomain::Starlet)==243000000ULL,"Starlet clock frequency");check(clock.nanoseconds_for_cycles(ClockDomain::Cpu,729000ULL)==1000000ULL,"cycle to nanosecond conversion");
 ClockDomainScheduler scheduler;
 const ClockDomainScheduler::Frequencies clock_rates{{729000000ULL,243000000ULL,162000000ULL,121500000ULL,243000000ULL}};
 const auto first_ticks=scheduler.advance(3U,729000000ULL,clock_rates);
 check(first_ticks[static_cast<std::size_t>(ClockDomain::Gpu)]==1U&&first_ticks[static_cast<std::size_t>(ClockDomain::Bus)]==0U,"clock scheduler preserves fractional domain ticks");
 const auto second_ticks=scheduler.advance(6U,729000000ULL,clock_rates);
 check(second_ticks[static_cast<std::size_t>(ClockDomain::Gpu)]==2U&&second_ticks[static_cast<std::size_t>(ClockDomain::Bus)]==2U&&second_ticks[static_cast<std::size_t>(ClockDomain::Dsp)]==1U,"clock scheduler carries fractions across advances");
 check(scheduler.total_cycles(ClockDomain::Starlet)==3U&&scheduler.total_cycles(ClockDomain::Cpu)==9U,"clock scheduler accumulates domain cycle totals");
 scheduler.reset();check(scheduler.total_cycles(ClockDomain::Gpu)==0U,"clock scheduler reset clears counters and phase");
 SystemClock invalid(std::array<std::uint64_t,5>{1,0,2,3,4});check(!invalid.valid()&&invalid.nanoseconds_for_cycles(ClockDomain::Gpu,100)==0U,"invalid clock domain frequency handled");
 InterruptController irq;irq.raise(3);irq.raise(1);check(irq.pending(3)&&irq.pending(1),"interrupt pending bits");check(irq.active_mask()==0U,"interrupts disabled by default");irq.set_enabled(3,true);irq.set_enabled(1,true);check(irq.active_mask()==((1U<<3U)|(1U<<1U)),"enabled pending mask");check(irq.claim_highest_priority()==1,"lowest numbered interrupt has priority");check(!irq.pending(1)&&irq.pending(3),"claim acknowledges only selected interrupt");irq.clear(3);check(irq.claim_highest_priority()==-1,"no active interrupt");irq.raise(32);irq.set_enabled(32,true);check(irq.pending_mask()==0U,"out-of-range interrupt ignored");
 TimerBank timers;InterruptController timer_irq;timers.enable(0,true);timers.advance(10,timer_irq);check(!timers.is_enabled(0)&&timers.expirations(0)==0U,"unconfigured timer cannot be enabled");check(!timers.configure(0,0,false,true),"zero reload rejected");check(!timers.configure(4,10,false,true),"invalid timer index rejected");check(timers.configure(0,10,false,true),"one-shot timer configured");timers.advance(9,timer_irq);check(timers.remaining(0)==1U&&timers.expirations(0)==0U,"timer counts down deterministically");timers.advance(1,timer_irq);check(timers.expirations(0)==1U&&!timers.is_enabled(0)&&timer_irq.pending(8),"one-shot expiry raises interrupt");timers.advance(100,timer_irq);check(timers.expirations(0)==1U,"one-shot does not repeat");
 check(timers.configure(1,5,true,true),"periodic timer configured");timers.advance(12,timer_irq);check(timers.expirations(1)==2U&&timers.remaining(1)==3U&&timer_irq.pending(9),"periodic timer accounts for multiple expiries");timers.advance(3,timer_irq);check(timers.expirations(1)==3U&&timers.remaining(1)==5U,"periodic timer reloads exactly");timers.enable(1,false);timers.advance(50,timer_irq);check(timers.expirations(1)==3U,"disabled timer stops");timers.enable(1,true);timers.advance(5,timer_irq);check(timers.expirations(1)==4U,"timer resumes");check(timers.remaining(99)==0U&&!timers.is_enabled(99),"invalid timer reads are safe");
 TimerBank huge;InterruptController huge_irq;check(huge.configure(0,1,true,true),"high frequency timer config");huge.advance(std::numeric_limits<std::uint64_t>::max(),huge_irq);check(huge.expirations(0)==std::numeric_limits<std::uint64_t>::max()&&huge.remaining(0)==1U,"expiration counter saturates");huge.advance(std::numeric_limits<std::uint64_t>::max(),huge_irq);check(huge.expirations(0)==std::numeric_limits<std::uint64_t>::max(),"expiration counter remains saturated");
 BootController boot;check(boot.state()==BootState::Off,"initial boot state");check(!boot.transition(BootState::Running),"invalid direct boot transition rejected");check(boot.transition(BootState::BootRom),"off to boot ROM");check(boot.transition(BootState::Firmware),"boot ROM to firmware");check(boot.transition(BootState::Running),"firmware to running");check(boot.transition(BootState::Halted),"running to halted");check(boot.transition(BootState::Running),"resume from halted");check(boot.transition(BootState::Faulted),"running to faulted");check(!boot.transition(BootState::Running),"faulted cannot resume directly");check(boot.transition(BootState::Off),"faulted to off reset");
 SystemHardware hardware;check(hardware.timers().configure(2,4,false,true),"system hardware timer setup");hardware.advance(4);check(hardware.interrupts().pending(10),"system hardware advances timers into interrupt controller");check(hardware.boot().transition(BootState::BootRom),"system boot controller");hardware.reset();check(hardware.boot().state()==BootState::Off&&!hardware.interrupts().pending(10)&&!hardware.timers().is_enabled(2),"system hardware reset clears state");
 // WiiSystem composition, peripheral MMIO routing and independent AHB gates.
 WiiSystem wii;check(!wii.initialized(),"WiiSystem begins uninitialized");check(wii.initialize()&&wii.initialized(),"WiiSystem initializes shared bus and peripherals");
 check(wii.memory().write32(0x80000000U,0x13579bdfU)==xavrx::memory::AccessResult::Success,"WiiSystem exposes MEM1");
 wii.advance_broadway_cycles(9U);
 check(wii.scheduled_cycles(ClockDomain::Starlet)==3U&&wii.hollywood().timer().counter()==3U,"WiiSystem derives Starlet ticks from Broadway cycles at 243 MHz");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000190U,4,2U)==xavrx::memory::AccessResult::Success,"WiiSystem switches Hollywood Starlet clock mode");
 wii.advance_broadway_cycles(9U);
 check(wii.scheduled_cycles(ClockDomain::Starlet)==5U&&wii.hollywood().timer().counter()==5U,"WiiSystem follows the 162 MHz Starlet clock mode");
 check(wii.scheduled_cycles(ClockDomain::Gpu)==6U&&wii.scheduled_cycles(ClockDomain::Dsp)==3U&&wii.scheduled_cycles(ClockDomain::Bus)==4U,"WiiSystem schedules GPU DSP and bus domains deterministically");
 std::uint64_t value=0;const std::uint32_t nand_addr=0x0d010000U,sd_addr=0x0d070008U,usb_addr=0x0d040010U;
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,nand_addr,4,0x11223344U)==xavrx::memory::AccessResult::DeviceError,"NAND denied while Starlet AHB permission is clear");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,sd_addr,4,0x55667788U)==xavrx::memory::AccessResult::DeviceError,"SD0 denied while Starlet AHB permission is clear");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,usb_addr,4,0xaabbccddU)==xavrx::memory::AccessResult::DeviceError,"EHCI denied while Starlet AHB permission is clear");
 const std::uint32_t iop_grants=(1U<<22U)|(1U<<28U)|(1U<<25U);
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000064U,4,iop_grants)==xavrx::memory::AccessResult::Success,"Starlet configures independent IOP engine permissions");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,nand_addr,4,0x11223344U)==xavrx::memory::AccessResult::Success,"Starlet accesses NAND after FlashNand permission");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,sd_addr,4,0x55667788U)==xavrx::memory::AccessResult::Success,"Starlet accesses SD after SD0 permission");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,usb_addr,4,0xaabbccddU)==xavrx::memory::AccessResult::Success,"Starlet accesses EHCI after permission");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,nand_addr,4,value)==xavrx::memory::AccessResult::Success&&value==0x11223344U,"NAND MMIO readback through Wii bus");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,sd_addr,4,value)==xavrx::memory::AccessResult::Success&&value==0x55667788U,"SD MMIO readback through Wii bus");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,usb_addr,4,value)==xavrx::memory::AccessResult::Success&&value==0xaabbccddU,"EHCI MMIO readback through Wii bus");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,sd_addr,4,1U)==xavrx::memory::AccessResult::DeviceError,"Broadway SD access remains denied without PPC permission");
 const std::uint32_t ppc_grants=(1U<<4U)|(1U<<7U)|(1U<<10U);
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000064U,4,iop_grants|ppc_grants)==xavrx::memory::AccessResult::Success,"Starlet grants matching PPC permissions");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,nand_addr,4,0x12345678U)==xavrx::memory::AccessResult::Success,"Broadway NAND access follows FlashNand PPC gate");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,sd_addr,4,0x23456789U)==xavrx::memory::AccessResult::Success,"Broadway SD access follows SD0 PPC gate");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,usb_addr,4,0x3456789aU)==xavrx::memory::AccessResult::Success,"Broadway EHCI access follows EHCI PPC gate");
 // DI reset lines gate the Drive Interface; both must be released.
 const std::uint32_t di_addr=0x0d806008U;
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,di_addr,4,0x01020304U)==xavrx::memory::AccessResult::DeviceError,"DI access denied while reset lines are asserted");
 const std::uint32_t di_release=(1U<<10U)|(1U<<3U);
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000194U,4,di_release)==xavrx::memory::AccessResult::Success,"Starlet releases both DI reset lines");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,di_addr,4,0x01020304U)==xavrx::memory::AccessResult::Success,"DI MMIO write after reset release");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,di_addr,4,value)==xavrx::memory::AccessResult::Success&&value==0x01020304U,"DI MMIO readback through Wii bus");
 // Internal WLAN uses SD1 AHB permissions, independently of the removable SD0 host.
 const std::uint32_t wlan_addr=0x0d080024U;
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,wlan_addr,4,0x00010000U)==xavrx::memory::AccessResult::DeviceError,"WLAN SDIO denied without SD1 permission");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000064U,4,iop_grants|(1U<<29U))==xavrx::memory::AccessResult::Success,"Starlet grants SD1 permission");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,wlan_addr,4,0x00010000U)==xavrx::memory::AccessResult::Success,"WLAN SDIO access after SD1 grant");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,wlan_addr,4,value)==xavrx::memory::AccessResult::Success&&value==0x00010000U,"WLAN SDIO register readback");
 // Bluetooth HCI packets travel over the USB1 path and obey its independent AHB gate.
 const std::array<std::uint8_t,4> hci_command{{0x01U,0x03U,0x0cU,0x00U}};
 const std::array<std::uint8_t,4> hci_event{{0x04U,0x0eU,0x01U,0x00U}};
 std::vector<std::uint8_t> hci_packet;
 check(!wii.submit_bluetooth_host_packet(HollywoodCpu::Starlet,hci_command),"Bluetooth host transport denied without USB1 permission");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000064U,4,iop_grants|(1U<<27U))==xavrx::memory::AccessResult::Success,"Starlet grants USB1 permission for Bluetooth HCI");
 check(wii.submit_bluetooth_host_packet(HollywoodCpu::Starlet,hci_command),"Bluetooth HCI command accepted through USB1");
 check(wii.take_bluetooth_device_packet(HollywoodCpu::Starlet,hci_packet)&&hci_packet.size()==hci_command.size()&&hci_packet[0]==0x01U,"Bluetooth HCI host packet delivered to device side");
 check(wii.submit_bluetooth_device_packet(hci_event),"Bluetooth device event enters host queue");
 check(wii.take_bluetooth_host_packet(HollywoodCpu::Starlet,hci_packet)&&hci_packet.size()==hci_event.size()&&hci_packet[0]==0x04U,"Bluetooth HCI event delivered to host side");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000064U,4,1U<<31U)==xavrx::memory::AccessResult::Success,"WiiSystem exposes PPCKERN control");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000190U,4,3U)==xavrx::memory::AccessResult::Success,"WiiSystem exposes Hollywood clock controls");

 // WiiSystem wires the reset register to the DSP scheduler and GPU FIFO.
 const std::uint32_t dsp_gpu_release=(1U<<26U)|(1U<<17U)|(1U<<16U);
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000194U,4,dsp_gpu_release)==xavrx::memory::AccessResult::Success,"Starlet releases DSP and graphics reset lines");
 wii.advance_broadway_cycles(0U);
 check(wii.reset_controller().released(HardwareResetLine::DSP)&&wii.reset_controller().released(HardwareResetLine::GFX),"WiiSystem synchronizes reset line state");
 check(wii.dsp().submit({0x55U,2U}),"WiiSystem accepts DSP task after reset release");
 const std::array<std::uint32_t,2> system_gx_words{{0x61000000U,0x00000003U}};
 check(wii.gpu().submit(system_gx_words),"WiiSystem accepts GPU command words");
 wii.advance_broadway_cycles(12U);
 check(wii.dsp().completed_tasks()==1U&&wii.dsp().last_completed_opcode()==0x55U,"WiiSystem advances DSP tasks from scheduled DSP clocks");
 check(wii.gpu().queued_words()==2U,"WiiSystem retains GPU FIFO while graphics reset is released");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000194U,4,1U<<26U)==xavrx::memory::AccessResult::Success,"Starlet asserts graphics reset while keeping DSP released");
 wii.advance_broadway_cycles(0U);
 check(wii.gpu().empty(),"asserting graphics reset clears GPU command FIFO");

 wii.reset();std::uint32_t reset_word=0;std::uint64_t reset_value=0;
 check(wii.initialized(),"WiiSystem remains mapped after reset");
 check(wii.memory().read32(0x80000000U,reset_word)==xavrx::memory::AccessResult::Success&&reset_word==0U,"WiiSystem reset clears MEM1 contents");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,0x0d000064U,4,reset_value)==xavrx::memory::AccessResult::Success&&reset_value==0U,"WiiSystem reset clears AHB protection");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,0x0d000190U,4,reset_value)==xavrx::memory::AccessResult::Success&&reset_value==0U,"WiiSystem reset clears Hollywood clocks");
 check(wii.scheduled_cycles(ClockDomain::Starlet)==0U&&wii.scheduled_cycles(ClockDomain::Gpu)==0U,"WiiSystem reset clears scheduler phase and counters");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,di_addr,4,1U)==xavrx::memory::AccessResult::DeviceError,"WiiSystem reset reasserts DI reset gates");
 check(!wii.submit_bluetooth_host_packet(HollywoodCpu::Starlet,hci_command),"WiiSystem reset clears USB1 permission");
 check(!wii.take_bluetooth_host_packet(HollywoodCpu::Starlet,hci_packet),"WiiSystem reset clears Bluetooth HCI queues");
 check(wii.memory().write32(0x80000000U,0x2468ace0U)==xavrx::memory::AccessResult::Success,"WiiSystem remains operational after reset");

 // Hardware reset controller: reset is active-low and reserved bit 31 is masked.
 HardwareResetController reset_controller;
 check(reset_controller.asserted(HardwareResetLine::DSP)&&reset_controller.asserted(HardwareResetLine::GFX),"hardware reset lines start asserted");
 const std::uint32_t released_lines=(1U<<26U)|(1U<<17U)|(1U<<16U)|(1U<<21U)|(1U<<14U);
 reset_controller.set_register(released_lines|0x80000000U);
 check(reset_controller.register_value()==released_lines&&reset_controller.released(HardwareResetLine::DSP)&&reset_controller.released(HardwareResetLine::GFX)&&reset_controller.released(HardwareResetLine::GFXTCPE),"reset controller releases selected lines and masks reserved bit");
 check(reset_controller.asserted(HardwareResetLine::IOP)&&reset_controller.released(HardwareResetLine::IODI),"reset controller tracks independent active-low lines");
 reset_controller.reset();check(reset_controller.register_value()==0U&&reset_controller.asserted(HardwareResetLine::DSP),"reset controller reset reasserts all modeled lines");
 // DSP task scheduler is deterministic scaffolding, not DSP instruction execution.
 DspDevice dsp_device;
 check(!dsp_device.submit({0x10U,0U}),"DSP rejects zero-cycle task");
 check(dsp_device.submit({0x1234U,5U})&&dsp_device.busy(),"DSP accepts bounded task");
 dsp_device.advance(2U);check(dsp_device.active_cycles_remaining()==3U&&dsp_device.completed_tasks()==0U,"DSP task retains remaining cycles");
 dsp_device.advance(3U);check(!dsp_device.busy()&&dsp_device.completed_tasks()==1U&&dsp_device.last_completed_opcode()==0x1234U,"DSP task completes at exact cycle boundary");
 dsp_device.reset();check(!dsp_device.busy()&&dsp_device.completed_tasks()==0U,"DSP reset clears task and completion state");
 // GPU command processor preserves FIFO order and rejects over-capacity batches atomically.
 GpuCommandProcessor gpu_device;const std::array<std::uint32_t,3> gx_words{{0x61000000U,0x00000001U,0x00000002U}};
 check(gpu_device.submit(gx_words)&&gpu_device.queued_words()==3U,"GPU accepts command words");
 std::uint32_t gx_word=0U;check(gpu_device.take(gx_word)&&gx_word==gx_words[0],"GPU command FIFO preserves first word");
 check(gpu_device.take(gx_word)&&gx_word==gx_words[1]&&gpu_device.take(gx_word)&&gx_word==gx_words[2],"GPU command FIFO preserves remaining order");
 check(!gpu_device.take(gx_word)&&gpu_device.empty(),"GPU empty FIFO rejects reads");
 const std::array<std::uint32_t,GpuCommandProcessor::fifo_capacity+1U> too_many{};
 check(!gpu_device.submit(too_many)&&gpu_device.empty(),"GPU rejects oversized command batch without partial enqueue");

 if(failures){std::cerr<<failures<<" system hardware test(s) failed\n";return 1;}std::cout<<"All XAVRX-WII system hardware tests passed\n";return 0;}
