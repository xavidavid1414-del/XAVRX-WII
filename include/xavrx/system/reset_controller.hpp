#pragma once
#include <cstdint>
namespace xavrx::system {
enum class HardwareResetLine : std::uint8_t {
 AHB, IOP, DSP, VI1, VI, IOPI, IOMEM, IODI, IOEXI, IOSI, AI_I2S3,
 GFX, GFXTCPE, MEM, DIRSTB, PI, MEMRSTB, SYSPLL_LOCK, SYSPLL,
 CPU_SOFT, CPU, DSKPLL, C, RSTBINB, Count
};
class HardwareResetController final {
public:
 static constexpr std::uint32_t writable_mask=0x7fffffffU;
 HardwareResetController() noexcept=default;
 explicit HardwareResetController(std::uint32_t value) noexcept { set_register(value); }
 void set_register(std::uint32_t value) noexcept { value_=value&writable_mask; }
 [[nodiscard]] std::uint32_t register_value() const noexcept{return value_;}
 [[nodiscard]] bool asserted(HardwareResetLine line) const noexcept;
 [[nodiscard]] bool released(HardwareResetLine line) const noexcept{return !asserted(line);}
 void reset() noexcept{value_=0U;}
private:
 [[nodiscard]] static std::uint32_t mask(HardwareResetLine line) noexcept;
 std::uint32_t value_{0U};
};
}
