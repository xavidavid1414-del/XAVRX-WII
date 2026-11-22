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
 SystemClock extreme_clock(std::array<std::uint64_t,5>{std::numeric_limits<std::uint64_t>::max(),std::numeric_limits<std::uint64_t>::max(),std::numeric_limits<std::uint64_t>::max(),std::numeric_limits<std::uint64_t>::max(),std::numeric_limits<std::uint64_t>::max()});
 check(extreme_clock.nanoseconds_for_cycles(ClockDomain::Cpu,std::numeric_limits<std::uint64_t>::max()-1U)==999999999ULL,"clock fractional conversion avoids multiplication overflow at extreme frequency");
 check(extreme_clock.nanoseconds_for_cycles(ClockDomain::Cpu,std::numeric_limits<std::uint64_t>::max())==1000000000ULL,"clock conversion handles exact one-second boundary at extreme frequency");
 InterruptController irq;irq.raise(3);irq.raise(1);check(irq.pending(3)&&irq.pending(1),"interrupt pending bits");check(irq.active_mask()==0U,"interrupts disabled by default");irq.set_enabled(3,true);irq.set_enabled(1,true);check(irq.active_mask()==((1U<<3U)|(1U<<1U)),"enabled pending mask");check(irq.claim_highest_priority()==1,"lowest numbered interrupt has priority");check(!irq.pending(1)&&irq.pending(3),"claim acknowledges only selected interrupt");irq.clear(3);check(irq.claim_highest_priority()==-1,"no active interrupt");irq.raise(32);irq.set_enabled(32,true);check(irq.pending_mask()==0U,"out-of-range interrupt ignored");
 TimerBank timers;InterruptController timer_irq;timers.enable(0,true);timers.advance(10,timer_irq);check(!timers.is_enabled(0)&&timers.expirations(0)==0U,"unconfigured timer cannot be enabled");check(!timers.configure(0,0,false,true),"zero reload rejected");check(!timers.configure(4,10,false,true),"invalid timer index rejected");check(timers.configure(0,10,false,true),"one-shot timer configured");timers.advance(9,timer_irq);check(timers.remaining(0)==1U&&timers.expirations(0)==0U,"timer counts down deterministically");timers.advance(1,timer_irq);check(timers.expirations(0)==1U&&!timers.is_enabled(0)&&timer_irq.pending(8),"one-shot expiry raises interrupt");timers.advance(100,timer_irq);check(timers.expirations(0)==1U,"one-shot does not repeat");
 check(timers.configure(1,5,true,true),"periodic timer configured");timers.advance(12,timer_irq);check(timers.expirations(1)==2U&&timers.remaining(1)==3U&&timer_irq.pending(9),"periodic timer accounts for multiple expiries");timers.advance(3,timer_irq);check(timers.expirations(1)==3U&&timers.remaining(1)==5U,"periodic timer reloads exactly");timers.enable(1,false);timers.advance(50,timer_irq);check(timers.expirations(1)==3U,"disabled timer stops");timers.enable(1,true);timers.advance(5,timer_irq);check(timers.expirations(1)==4U,"timer resumes");check(timers.remaining(99)==0U&&!timers.is_enabled(99),"invalid timer reads are safe");
 TimerBank huge;InterruptController huge_irq;check(huge.configure(0,1,true,true),"high frequency timer config");huge.advance(std::numeric_limits<std::uint64_t>::max(),huge_irq);check(huge.expirations(0)==std::numeric_limits<std::uint64_t>::max()&&huge.remaining(0)==1U,"expiration counter saturates");huge.advance(std::numeric_limits<std::uint64_t>::max(),huge_irq);check(huge.expirations(0)==std::numeric_limits<std::uint64_t>::max(),"expiration counter remains saturated");
 BootController boot;check(boot.state()==BootState::Off,"initial boot state");check(!boot.transition(BootState::Running),"invalid direct boot transition rejected");check(boot.transition(BootState::BootRom),"off to boot ROM");check(boot.transition(BootState::Firmware),"boot ROM to firmware");check(boot.transition(BootState::Running),"firmware to running");check(boot.transition(BootState::Halted),"running to halted");check(boot.transition(BootState::Running),"resume from halted");check(boot.transition(BootState::Faulted),"running to faulted");check(!boot.transition(BootState::Running),"faulted cannot resume directly");check(boot.transition(BootState::Off),"faulted to off reset");
 SystemHardware hardware;check(hardware.timers().configure(2,4,false,true),"system hardware timer setup");hardware.advance(4);check(hardware.interrupts().pending(10),"system hardware advances timers into interrupt controller");check(hardware.boot().transition(BootState::BootRom),"system boot controller");hardware.reset();check(hardware.boot().state()==BootState::Off&&!hardware.interrupts().pending(10)&&!hardware.timers().is_enabled(2),"system hardware reset clears state");
 // Starlet SRAM access rejects unknown CPU identities instead of treating them as Starlet.
 StarletMemory starlet_memory;
 const auto invalid_starlet_cpu=static_cast<StarletMemoryCpu>(0xffU);
 std::uint64_t starlet_probe=0xfeedfaceU;
 check(!starlet_memory.read_as(invalid_starlet_cpu,StarletMemory::sram_base,4U,starlet_probe)&&starlet_probe==0xfeedfaceU,"Starlet SRAM read rejects invalid CPU identity without changing output");
 check(!starlet_memory.write_as(invalid_starlet_cpu,StarletMemory::sram_base,4U,0x12345678U),"Starlet SRAM write rejects invalid CPU identity");
 check(!starlet_memory.master_access_allowed(static_cast<StarletSramMaster>(0xffU)),"Starlet SRAM master permission rejects invalid master identity");
 // WiiSystem composition, peripheral MMIO routing and independent AHB gates.
 WiiSystem wii;check(!wii.initialized(),"WiiSystem begins uninitialized");check(wii.initialize()&&wii.initialized(),"WiiSystem initializes shared bus and peripherals");
 // DI image reads copy bytes into the shared memory map for both processors.
 std::vector<std::uint8_t> di_image(32U);
 for(std::size_t i=0;i<di_image.size();++i)di_image[i]=static_cast<std::uint8_t>(i+1U);
 check(wii.drive_interface().mount_disc_image(di_image),"WiiSystem mounts DI image for DMA");
 check(wii.read_disc_to_memory(WiiMemoryCpu::Broadway,0U,0x80001000U,32U),"DI transfers image bytes into Broadway MEM1");
 std::uint64_t di_byte=0U;
 check(wii.memory().read_as(WiiMemoryCpu::Broadway,0x80001000U,4U,di_byte)==xavrx::memory::AccessResult::Success&&di_byte==0x01020304U,"DI DMA preserves big-endian byte order");
 check(wii.read_disc_to_memory(WiiMemoryCpu::Starlet,0U,0x80002000U,32U),"DI transfers image bytes into Starlet-visible MEM1");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,0x80002000U,4U,di_byte)==xavrx::memory::AccessResult::Success&&di_byte==0x01020304U,"Starlet observes DI DMA data in shared MEM1");
 check(!wii.read_disc_to_memory(WiiMemoryCpu::Broadway,31U,0x80003000U,2U),"DI DMA rejects reads past image end");
 check(!wii.read_disc_to_memory(static_cast<WiiMemoryCpu>(0xffU),0U,0x80003000U,1U),"DI DMA rejects invalid CPU identity");
 check(!wii.read_disc_to_memory(WiiMemoryCpu::Broadway,0U,0xffffffffU,2U),"DI DMA rejects destination address overflow before writing");
 check(!wii.read_disc_to_memory(WiiMemoryCpu::Broadway,0U,0x817ffffcU,8U),"DI DMA rejects a transfer crossing the MEM1 boundary");
 check(!wii.read_disc_to_memory(WiiMemoryCpu::Broadway,0U,0x0d010000U,4U),"DI DMA rejects MMIO destinations instead of partially writing registers");
 check(wii.memory().read_as(WiiMemoryCpu::Broadway,0x80003000U,1U,di_byte)==xavrx::memory::AccessResult::Success&&di_byte==0U,"rejected DI DMA does not alter destination");
 check(wii.memory().read_as(WiiMemoryCpu::Broadway,0x817ffffcU,4U,di_byte)==xavrx::memory::AccessResult::Success&&di_byte==0U,"rejected cross-bank DI DMA leaves the final MEM1 bytes unchanged");

 // The composed system routes live states from every controller profile to game actions.
 check(wii.input_router().add_binding({ControllerType::WiiRemote,InputControl::RemoteA,GameAction::Confirm}),"WiiSystem registers Wii Remote binding");
 check(wii.input_router().add_binding({ControllerType::WiiRemote,InputControl::PointerVisible,GameAction::PointerVisible}),"WiiSystem registers pointer visibility binding");
 check(wii.input_router().add_binding({ControllerType::Nunchuk,InputControl::NunchukStickX,GameAction::MoveX,1.0F,0.0F,false}),"WiiSystem registers Nunchuk axis binding");
 check(wii.input_router().add_binding({ControllerType::Classic,InputControl::ClassicLeftX,GameAction::MoveX,1.0F,0.0F,false}),"WiiSystem registers Classic Controller axis binding");
 check(wii.input_router().add_binding({ControllerType::ClassicPro,InputControl::ProRightX,GameAction::CameraX,1.0F,0.0F,false}),"WiiSystem registers Classic Pro axis binding");
 check(wii.input_router().add_binding({ControllerType::BalanceBoard,InputControl::BalanceWeightKg,GameAction::WeightKg,1.0F,0.0F,false}),"WiiSystem registers Balance Board weight binding");
 (void)wii.wii_remote().set_button(WiiRemoteButton::A,true);wii.wii_remote().set_pointer(0.2F,0.8F,true);wii.nunchuk().set_stick(-0.9F,0.0F);wii.classic_controller().set_sticks(-0.5F,0.0F,0.0F,0.0F);wii.classic_controller_pro().set_sticks(0.0F,0.0F,0.75F,0.0F);
 for(unsigned i=0;i<4U;++i)(void)wii.balance_board().set_corner_calibration(static_cast<BalanceBoardCorner>(i),0.0F,0.01F);
 wii.balance_board().set_sensor_readings(1000.0F,1000.0F,1000.0F,1000.0F);
 const auto routed_inputs=wii.route_controller_inputs();
 check(routed_inputs.pressed(GameAction::Confirm),"WiiSystem routes live Wii Remote buttons");
 check(routed_inputs.pressed(GameAction::PointerVisible),"WiiSystem routes live Wii Remote pointer visibility");
 check(routed_inputs.value(GameAction::CameraX)>0.74F&&routed_inputs.value(GameAction::CameraX)<0.76F,"WiiSystem routes Classic Pro right stick");
 check(routed_inputs.value(GameAction::MoveX)<-0.89F&&routed_inputs.value(GameAction::MoveX)>-0.91F,"WiiSystem selects the strongest absolute analog axis across profiles");
 check(routed_inputs.value(GameAction::WeightKg)>0.26F&&routed_inputs.value(GameAction::WeightKg)<0.27F,"WiiSystem routes calibrated Balance Board weight");
 // Broadway-only DSP mailbox, PI interrupt and GX write-gather mappings.
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,0x0c005000U,2,0x9234U)==xavrx::memory::AccessResult::Success,"Broadway writes DSP mailbox high halfword");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,0x0c005002U,2,0x5678U)==xavrx::memory::AccessResult::Success,"Broadway writes DSP mailbox low halfword");
 std::uint32_t dsp_mail=0;check(wii.dsp_mailbox().take_mail_to_dsp(dsp_mail)&&dsp_mail==0x12345678U,"WiiSystem exposes DSP mailbox payload");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,0x0c003004U,4,1U<<6U)==xavrx::memory::AccessResult::Success,"Broadway configures PI interrupt mask");
 wii.processor_interrupts().raise(6U);check(wii.processor_interrupts().active()==(1U<<6U),"PI interrupt source is masked and observable");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,0x0c003000U,4,1U<<6U)==xavrx::memory::AccessResult::Success&&wii.processor_interrupts().cause()==0U,"PI interrupt cause is W1C through MMIO");
 check(wii.memory().write_as(WiiMemoryCpu::Broadway,0x0c008000U,4,0xabcdef01U)==xavrx::memory::AccessResult::Success,"Broadway GX write-gather port accepts FIFO word");
 std::uint32_t gx_mmio_word=0;check(wii.gpu().take(gx_mmio_word)&&gx_mmio_word==0xabcdef01U,"GX FIFO receives MMIO write-gather word");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0c008000U,4,0U)==xavrx::memory::AccessResult::DeviceError,"Starlet cannot access Broadway-only GX FIFO");
 check(wii.memory().write32(0x80000000U,0x13579bdfU)==xavrx::memory::AccessResult::Success,"WiiSystem exposes MEM1");
 wii.advance_broadway_cycles(9U);
 check(wii.scheduled_cycles(ClockDomain::Starlet)==3U&&wii.hollywood().timer().counter()==3U,"WiiSystem derives Starlet ticks from Broadway cycles at 243 MHz");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000190U,4,2U)==xavrx::memory::AccessResult::Success,"WiiSystem switches Hollywood Starlet clock mode");
 wii.advance_broadway_cycles(9U);
 check(wii.scheduled_cycles(ClockDomain::Starlet)==5U&&wii.hollywood().timer().counter()==5U,"WiiSystem follows the 162 MHz Starlet clock mode");
 check(wii.scheduled_cycles(ClockDomain::Gpu)==6U&&wii.scheduled_cycles(ClockDomain::Dsp)==3U&&wii.scheduled_cycles(ClockDomain::Bus)==4U,"WiiSystem schedules GPU DSP and bus domains deterministically");
 auto& integrated_usb=wii.usb_host();integrated_usb.reset();integrated_usb.connect_device(true);
 check(integrated_usb.write(0x54U,4,0x100U)&&integrated_usb.complete_port_reset(true),"WiiSystem USB integration completes high-speed port reset");
 const std::array<std::uint8_t,2> usb_payload{{0x5aU,0xa5U}};
 const std::uint32_t ehci_irq=1U<<4U;
 wii.hollywood().interrupts().set_mask(HollywoodCpu::Broadway,ehci_irq);
 wii.hollywood().interrupts().set_mask(HollywoodCpu::Starlet,ehci_irq);
 check(integrated_usb.write(0x18U,4,0x5U),"WiiSystem enables EHCI USB and port-change interrupts");
 check(integrated_usb.write(0x10U,4,1U)&&integrated_usb.enqueue_transfer(0x7788U,usb_payload),"WiiSystem USB integration queues a running transfer");
 wii.advance_broadway_cycles(91106U);
 check(integrated_usb.pending_transfers()==1U&&integrated_usb.completed_transfers()==0U,"WiiSystem USB transfer waits until a complete 125us microframe");
 check((wii.hollywood().interrupts().flags(HollywoodCpu::Broadway)&ehci_irq)!=0U&&(wii.hollywood().interrupts().flags(HollywoodCpu::Starlet)&ehci_irq)!=0U,"WiiSystem routes EHCI port-change IRQ 4 to Broadway and Starlet");
 check(integrated_usb.write(0x14U,4,0x4U),"WiiSystem acknowledges EHCI port-change status");
 wii.hollywood().interrupts().clear(HollywoodCpu::Broadway,ehci_irq);
 wii.hollywood().interrupts().clear(HollywoodCpu::Starlet,ehci_irq);
 wii.advance_broadway_cycles(1U);
 UsbHostController::Transfer integrated_transfer;
 check(integrated_usb.take_completed_transfer(integrated_transfer)&&integrated_transfer.token==0x7788U&&integrated_transfer.payload==std::vector<std::uint8_t>(usb_payload.begin(),usb_payload.end()),"WiiSystem advances EHCI from bus clock and completes transfer at microframe boundary");
 check((wii.hollywood().interrupts().flags(HollywoodCpu::Broadway)&ehci_irq)!=0U&&(wii.hollywood().interrupts().flags(HollywoodCpu::Starlet)&ehci_irq)!=0U,"WiiSystem routes enabled EHCI completion interrupt to both CPUs");
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
 // A documented DI 0xA8 read command is dispatched by DICR and DMA-writes shared MEM1.
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806008U,4,0xa8000000U)==xavrx::memory::AccessResult::Success,"DI command buffer 0 accepts read command");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80600cU,4,0U)==xavrx::memory::AccessResult::Success,"DI command buffer 1 sets image byte offset");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806010U,4,32U)==xavrx::memory::AccessResult::Success,"DI command buffer 2 sets transfer size");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806014U,4,0x80004000U)==xavrx::memory::AccessResult::Success,"DI DMA destination configured");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806018U,4,32U)==xavrx::memory::AccessResult::Success,"DI DMA length configured");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,3U)==xavrx::memory::AccessResult::Success,"DI DICR starts supported read DMA command");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,0x80004000U,4,value)==xavrx::memory::AccessResult::Success&&value==0x01020304U,"DI register command transfers image bytes into shared MEM1");
 check(wii.memory().read_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,value)==xavrx::memory::AccessResult::Success&&(value&1U)==0U,"DI command start bit self-clears after synchronous completion");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806018U,4,8U)==xavrx::memory::AccessResult::Success,"DI non-aligned transfer length configured");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,3U)==xavrx::memory::AccessResult::DeviceError,"DI rejects non-32-byte DMA length");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806018U,4,32U)==xavrx::memory::AccessResult::Success,"DI restores aligned transfer length");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806014U,4,0x80004004U)==xavrx::memory::AccessResult::Success,"DI unaligned DMA destination configured");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,3U)==xavrx::memory::AccessResult::DeviceError,"DI rejects unaligned DMA destination");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806014U,4,0x80004000U)==xavrx::memory::AccessResult::Success,"DI restores aligned DMA destination");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,7U)==xavrx::memory::AccessResult::DeviceError,"DI rejects unsupported memory-to-disc DMA direction");
 // Unsupported command and invalid destination are rejected without changing RAM.
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806008U,4,0x12000000U)==xavrx::memory::AccessResult::Success,"DI stores an unsupported command for rejection test");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,3U)==xavrx::memory::AccessResult::DeviceError,"DI rejects unsupported command opcode");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806008U,4,0xa8000000U)==xavrx::memory::AccessResult::Success,"DI restores supported read opcode");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d806014U,4,0x0d010000U)==xavrx::memory::AccessResult::Success,"DI invalid MMIO destination configured");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d80601cU,4,3U)==xavrx::memory::AccessResult::DeviceError,"DI rejects MMIO DMA destination atomically");
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
 std::vector<std::uint8_t> hci_response;
 check(!wii.process_bluetooth_host_command(HollywoodCpu::Starlet,hci_command,hci_response),"Bluetooth HCI command execution denied without USB1 permission");
 check(wii.memory().write_as(WiiMemoryCpu::Starlet,0x0d000064U,4,iop_grants|(1U<<27U))==xavrx::memory::AccessResult::Success,"Starlet grants USB1 permission for Bluetooth HCI");
 check(wii.submit_bluetooth_host_packet(HollywoodCpu::Starlet,hci_command),"Bluetooth HCI command accepted through USB1");
 check(wii.take_bluetooth_device_packet(HollywoodCpu::Starlet,hci_packet)&&hci_packet.size()==hci_command.size()&&hci_packet[0]==0x01U,"Bluetooth HCI host packet delivered to device side");
 check(wii.process_bluetooth_host_command(HollywoodCpu::Starlet,hci_command,hci_response)&&hci_response.size()==7U&&hci_response[0]==0x04U&&hci_response[1]==0x0eU&&hci_response[6]==0U,"WiiSystem executes Bluetooth HCI Reset and returns Command Complete");
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
 check(dsp_device.submit({0x20U,2U})&&dsp_device.submit({0x21U,3U}),"DSP queues consecutive tasks");
 dsp_device.advance(4U);check(dsp_device.completed_tasks()==1U&&dsp_device.last_completed_opcode()==0x20U&&dsp_device.active_cycles_remaining()==1U,"DSP advance crosses task boundary and preserves next task remainder");
 dsp_device.advance(1U);check(dsp_device.completed_tasks()==2U&&dsp_device.last_completed_opcode()==0x21U&&!dsp_device.busy(),"DSP completes queued tasks in order");
 for(std::size_t i=0;i<DspDevice::task_capacity;++i)check(dsp_device.submit({static_cast<std::uint32_t>(i),1U}),"DSP task queue accepts capacity");
 check(!dsp_device.submit({0xffffffffU,1U})&&dsp_device.queued_tasks()==DspDevice::task_capacity,"DSP task queue rejects overflow without mutation");
 dsp_device.reset();check(dsp_device.queued_tasks()==0U,"DSP reset clears a full task queue");
 // GPU command processor preserves FIFO order and rejects over-capacity batches atomically.
 GpuCommandProcessor gpu_device;const std::array<std::uint32_t,3> gx_words{{0x61000000U,0x00000001U,0x00000002U}};
 check(gpu_device.submit(gx_words)&&gpu_device.queued_words()==3U,"GPU accepts command words");
 std::uint32_t gx_word=0U;check(gpu_device.take(gx_word)&&gx_word==gx_words[0],"GPU command FIFO preserves first word");
 check(gpu_device.take(gx_word)&&gx_word==gx_words[1]&&gpu_device.take(gx_word)&&gx_word==gx_words[2],"GPU command FIFO preserves remaining order");
 check(!gpu_device.take(gx_word)&&gpu_device.empty(),"GPU empty FIFO rejects reads");
 const std::array<std::uint32_t,GpuCommandProcessor::fifo_capacity+1U> too_many{};
 check(!gpu_device.submit(too_many)&&gpu_device.empty(),"GPU rejects oversized command batch without partial enqueue");
 std::array<std::uint32_t,GpuCommandProcessor::fifo_capacity> full_fifo{};
 check(gpu_device.submit(full_fifo)&&gpu_device.full(),"GPU FIFO accepts exact capacity");
 check(!gpu_device.submit(gx_words)&&gpu_device.queued_words()==GpuCommandProcessor::fifo_capacity,"GPU FIFO rejects overflow atomically");
 check(gpu_device.take(gx_word)&&!gpu_device.full()&&gpu_device.queued_words()==GpuCommandProcessor::fifo_capacity-1U,"GPU FIFO frees capacity after consuming one word");
 gpu_device.reset();check(gpu_device.empty(),"GPU reset clears full FIFO");

 if(failures){std::cerr<<failures<<" system hardware test(s) failed\n";return 1;}std::cout<<"All XAVRX-WII system hardware tests passed\n";return 0;}
