#pragma once
#include "xavrx/memory/memory.hpp"
#include "xavrx/system/hardware.hpp"
#include "xavrx/system/wii_memory.hpp"
#include "xavrx/system/starlet_memory.hpp"
#include <cstddef>
#include <cstdint>
namespace xavrx::system {
enum class HollywoodCpu : std::uint8_t { Broadway, Starlet };
enum class HollywoodEngine : std::uint8_t { Sd0, Sd1, Usb0, Usb1, Ehci, Sha, Aes, FlashNand };
enum class IrqRoute : std::uint8_t { Broadway=1, Starlet=2, Both=3 };
class HollywoodInterrupts final {
public:
 static constexpr std::uint32_t ppc_flag=0x30,ppc_mask=0x34,arm_flag=0x38,arm_mask=0x3c;
 void raise(std::uint8_t source,IrqRoute route=IrqRoute::Both) noexcept;
 void clear(HollywoodCpu cpu,std::uint32_t bits) noexcept;
 void set_mask(HollywoodCpu cpu,std::uint32_t bits) noexcept;
 void set_fiq_mask(std::uint32_t bits) noexcept{arm_fiq_mask_=bits;}
 [[nodiscard]] std::uint32_t flags(HollywoodCpu cpu) const noexcept;
 [[nodiscard]] std::uint32_t mask(HollywoodCpu cpu) const noexcept;
 [[nodiscard]] bool asserted(HollywoodCpu cpu) const noexcept;
 [[nodiscard]] std::uint32_t fiq_mask()const noexcept{return arm_fiq_mask_;}
 [[nodiscard]] bool fiq_asserted()const noexcept{return (arm_flags_&arm_fiq_mask_)!=0U;}
private:
 std::uint32_t ppc_flags_{0},ppc_mask_{0},arm_flags_{0},arm_mask_{0},arm_fiq_mask_{0};
};
class HollywoodTimer final {
public:
 static constexpr std::uint32_t timer_register=0x10,alarm_register=0x14;
 explicit HollywoodTimer(HollywoodInterrupts& irq) noexcept:irq_(irq){}
 [[nodiscard]] std::uint32_t counter() const noexcept{return counter_;}
 [[nodiscard]] bool write_alarm(std::uint32_t deadline) noexcept;
 void advance(std::uint64_t ticks) noexcept;
 void reset() noexcept{counter_=0;alarm_=0;armed_=false;}
 [[nodiscard]] bool armed() const noexcept{return armed_;}
private:
 HollywoodInterrupts& irq_;
 std::uint32_t counter_{0},alarm_{0};
 bool armed_{false};
};
class HollywoodSystemControl final {
public:
 static constexpr std::uint32_t srnprot_register=0x60,ahb_prot_register=0x64,boot0_register=0x18c,clocks_register=0x190,resets_register=0x194;
 static constexpr std::uint32_t fx_bit=1U<<0U,speed_bit=1U<<1U,resets_writable_mask=0x7fffffffU,di_reset_bit=1U<<10U;
 [[nodiscard]] bool read(HollywoodCpu cpu,std::uint32_t offset,std::uint32_t& value)const noexcept;
 [[nodiscard]] bool write(HollywoodCpu cpu,std::uint32_t offset,std::uint32_t value)noexcept;
 [[nodiscard]] bool ppc_full_access()const noexcept{return (ahb_prot_&(1U<<31U))!=0U;}
 [[nodiscard]] bool engine_access_allowed(HollywoodCpu initiator,HollywoodEngine engine)const noexcept;
 [[nodiscard]] std::uint32_t ahb_protection()const noexcept{return ahb_prot_;}
 [[nodiscard]] std::uint64_t hollywood_clock_hz()const noexcept{return (clocks_&speed_bit)!=0U?162000000ULL:243000000ULL;}
 [[nodiscard]] bool fx_enabled()const noexcept{return (clocks_&fx_bit)!=0U;}
 [[nodiscard]] std::uint32_t reset_register()const noexcept{return resets_;}
 [[nodiscard]] bool reset_released(std::uint32_t mask)const noexcept{return (resets_&mask)==mask;}\n [[nodiscard]] bool di_reset_asserted()const noexcept{return (resets_&di_reset_bit)==0U;}
 void reset()noexcept{ahb_prot_=0;clocks_=0;resets_=0;}
private:
 std::uint32_t ahb_prot_{0},clocks_{0},resets_{0};
};
class HollywoodIpc final {
public:
 static constexpr std::uint32_t ppc_msg=0,ppc_ctrl=4,arm_msg=8,arm_ctrl=12;
 explicit HollywoodIpc(HollywoodInterrupts& irq) noexcept:irq_(irq){}
 [[nodiscard]] bool read(HollywoodCpu cpu,std::uint32_t offset,std::uint32_t& value) const noexcept;
 [[nodiscard]] bool write(HollywoodCpu cpu,std::uint32_t offset,std::uint32_t value) noexcept;
 void reset() noexcept;
private:
 void update_interrupts() noexcept;
 HollywoodInterrupts& irq_;
 std::uint32_t ppc_message_{0},arm_message_{0};
 std::uint8_t x1_{0},x2_{0},y1_{0},y2_{0},iy1_{0},iy2_{0},ix1_{0},ix2_{0};
};
class HollywoodGpio final {
public:
 static constexpr std::size_t pin_count=24;
 explicit HollywoodGpio(HollywoodInterrupts& irq) noexcept:irq_(irq){}
 [[nodiscard]] bool read(HollywoodCpu cpu,std::uint32_t offset,std::uint32_t& value) const noexcept;
 [[nodiscard]] bool write(HollywoodCpu cpu,std::uint32_t offset,std::uint32_t value) noexcept;
 void set_inputs(std::uint32_t levels) noexcept;
 void reset() noexcept;
private:
 void refresh_interrupts() noexcept;
 HollywoodInterrupts& irq_;
 std::uint32_t input_{0},straps_{0},owner_{0},enable_{0x00ffffffU};
 bool inputs_initialized_{false};
 std::uint32_t output_{0},direction_{0},interrupt_level_{0},interrupt_flags_{0},interrupt_mask_{0};
};
class HollywoodHardware final : public memory::MemoryMappedDevice {
public:
 static constexpr std::uint32_t base=0x0d000000U,mirror=0x0d800000U,window_size=0x400U;
 class CpuBusView final : public memory::MemoryMappedDevice {
 public:
  CpuBusView(HollywoodHardware& hardware,HollywoodCpu cpu) noexcept:hardware_(hardware),cpu_(cpu){}
  [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
  [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 private:
  HollywoodHardware& hardware_;
  HollywoodCpu cpu_;
 };
 HollywoodHardware() noexcept:ipc_(interrupts_),gpio_(interrupts_),timer_(interrupts_),broadway_view_(*this,HollywoodCpu::Broadway),starlet_view_(*this,HollywoodCpu::Starlet){}
 [[nodiscard]] HollywoodInterrupts& interrupts() noexcept{return interrupts_;}
 [[nodiscard]] HollywoodIpc& ipc() noexcept{return ipc_;}
 [[nodiscard]] HollywoodGpio& gpio() noexcept{return gpio_;}
 [[nodiscard]] HollywoodTimer& timer() noexcept{return timer_;}
 [[nodiscard]] HollywoodSystemControl& system_control() noexcept{return system_control_;}
 [[nodiscard]] StarletMemory& starlet_memory() noexcept{return starlet_memory_;}
 void advance_timer(std::uint64_t ticks) noexcept{timer_.advance(ticks);}
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 [[nodiscard]] bool read_as(HollywoodCpu cpu,std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept;
 [[nodiscard]] bool write_as(HollywoodCpu cpu,std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept;
 [[nodiscard]] bool map_into(memory::Memory& broadway_bus,memory::Memory& starlet_bus) noexcept;
 [[nodiscard]] bool map_into(WiiMemory& shared_bus) noexcept;
 void reset() noexcept;
private:
 HollywoodInterrupts interrupts_{};
 HollywoodIpc ipc_;
 HollywoodGpio gpio_;
 HollywoodTimer timer_;
 HollywoodSystemControl system_control_{};
 StarletMemory starlet_memory_{};
 CpuBusView broadway_view_;
 CpuBusView starlet_view_;
};
}
