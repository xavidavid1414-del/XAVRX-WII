#pragma once
#include "xavrx/memory/memory.hpp"
#include "xavrx/system/hardware.hpp"
#include <array>
#include <cstdint>
namespace xavrx::system {
enum class HollywoodCpu : std::uint8_t { Broadway, Starlet };
enum class IrqRoute : std::uint8_t { Broadway=1, Starlet=2, Both=3 };
class HollywoodInterrupts final {
public:
 static constexpr std::uint32_t ppc_flag=0x30,ppc_mask=0x34,arm_flag=0x38,arm_mask=0x3c;
 void raise(std::uint8_t source,IrqRoute route=IrqRoute::Both) noexcept;
 void clear(HollywoodCpu cpu,std::uint32_t bits) noexcept;
 void set_mask(HollywoodCpu cpu,std::uint32_t bits) noexcept;
 [[nodiscard]] std::uint32_t flags(HollywoodCpu cpu) const noexcept;
 [[nodiscard]] std::uint32_t mask(HollywoodCpu cpu) const noexcept;
 [[nodiscard]] bool asserted(HollywoodCpu cpu) const noexcept;
private:
 std::uint32_t ppc_flags_{0},ppc_mask_{0},arm_flags_{0},arm_mask_{0};
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
 std::uint32_t b_out_{0},b_dir_{0},b_level_{0},b_flags_{0},b_mask_{0};
 std::uint32_t a_out_{0},a_dir_{0},a_level_{0},a_flags_{0},a_mask_{0};
};
class HollywoodHardware final : public memory::MemoryMappedDevice {
public:
 static constexpr std::uint32_t base=0x0d000000U,mirror=0x0d800000U,window_size=0x400U;
 HollywoodHardware() noexcept:ipc_(interrupts_),gpio_(interrupts_){}
 [[nodiscard]] HollywoodInterrupts& interrupts() noexcept{return interrupts_;}
 [[nodiscard]] HollywoodIpc& ipc() noexcept{return ipc_;}
 [[nodiscard]] HollywoodGpio& gpio() noexcept{return gpio_;}
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 [[nodiscard]] bool read_as(HollywoodCpu cpu,std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept;
 [[nodiscard]] bool write_as(HollywoodCpu cpu,std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept;
 [[nodiscard]] bool map_into(memory::Memory& bus) noexcept;
 void reset() noexcept;
private:
 HollywoodInterrupts interrupts_{};
 HollywoodIpc ipc_;
 HollywoodGpio gpio_;
};
}
