#include "xavrx/system/reset_controller.hpp"
namespace xavrx::system {
std::uint32_t HardwareResetController::mask(HardwareResetLine line) noexcept {
 switch(line){
 case HardwareResetLine::AHB:return 1U<<28U;
 case HardwareResetLine::IOP:return 1U<<27U;
 case HardwareResetLine::DSP:return 1U<<26U;
 case HardwareResetLine::VI1:return 1U<<25U;
 case HardwareResetLine::VI:return 1U<<24U;
 case HardwareResetLine::IOPI:return 1U<<23U;
 case HardwareResetLine::IOMEM:return 1U<<22U;
 case HardwareResetLine::IODI:return 1U<<21U;
 case HardwareResetLine::IOEXI:return 1U<<20U;
 case HardwareResetLine::IOSI:return 1U<<19U;
 case HardwareResetLine::AI_I2S3:return 1U<<18U;
 case HardwareResetLine::GFX:return 1U<<17U;
 case HardwareResetLine::GFXTCPE:return 1U<<16U;
 case HardwareResetLine::MEM:return 1U<<15U;
 case HardwareResetLine::DIRSTB:return 1U<<14U;
 case HardwareResetLine::PI:return 1U<<13U;
 case HardwareResetLine::MEMRSTB:return 1U<<12U;
 case HardwareResetLine::SYSPLL_LOCK:return 1U<<11U;
 case HardwareResetLine::SYSPLL:return 1U<<10U;
 case HardwareResetLine::CPU_SOFT:return 1U<<9U;
 case HardwareResetLine::CPU:return 1U<<8U;
 case HardwareResetLine::DSKPLL:return 1U<<7U;
 case HardwareResetLine::C:return 1U<<5U;
 case HardwareResetLine::RSTBINB:return 1U<<4U;
 case HardwareResetLine::Count:break;
 }
 return 0U;
}
bool HardwareResetController::asserted(HardwareResetLine line) const noexcept {
 const auto bit=mask(line);return bit!=0U&&(value_&bit)==0U;
}
}
