#include "xavrx/system/controllers.hpp"
#include <algorithm>
#include <cmath>

namespace xavrx::system {
namespace {
template <typename Enum>
[[nodiscard]] constexpr bool valid_button(Enum value, unsigned count) noexcept {
 return static_cast<unsigned>(value)<count;
}
[[nodiscard]] float clamp_axis(float value) noexcept {
 if(!std::isfinite(value)) return 0.0F;
 return std::clamp(value,-1.0F,1.0F);
}
[[nodiscard]] float clamp_trigger(float value) noexcept {
 if(!std::isfinite(value)) return 0.0F;
 return std::clamp(value,0.0F,1.0F);
}
[[nodiscard]] float clamp_pointer(float value) noexcept {
 if(!std::isfinite(value)) return 0.5F;
 return std::clamp(value,0.0F,1.0F);
}
void set_bit(std::uint16_t& bits,unsigned index,bool pressed) noexcept {
 const auto mask=static_cast<std::uint16_t>(1U<<index);
 if(pressed) bits=static_cast<std::uint16_t>(bits|mask);
 else bits=static_cast<std::uint16_t>(bits&static_cast<std::uint16_t>(~mask));
}
void set_bit(std::uint8_t& bits,unsigned index,bool pressed) noexcept {
 const auto mask=static_cast<std::uint8_t>(1U<<index);
 if(pressed) bits=static_cast<std::uint8_t>(bits|mask);
 else bits=static_cast<std::uint8_t>(bits&static_cast<std::uint8_t>(~mask));
}
}
bool WiiRemoteController::set_button(WiiRemoteButton button_value,bool pressed) noexcept {
 if(!valid_button(button_value,11U)) return false;
 set_bit(state_.buttons,static_cast<unsigned>(button_value),pressed); return true;
}
bool WiiRemoteController::button(WiiRemoteButton button_value) const noexcept {
 return valid_button(button_value,11U)&&(state_.buttons&(1U<<static_cast<unsigned>(button_value)))!=0U;
}
void WiiRemoteController::set_pointer(float x,float y,bool visible) noexcept {
 state_.pointer={clamp_pointer(x),clamp_pointer(y),visible};
}
void WiiRemoteController::set_acceleration(float x,float y,float z) noexcept {
 state_.acceleration={clamp_axis(x),clamp_axis(y),clamp_axis(z)};
}
bool NunchukController::set_button(NunchukButton button_value,bool pressed) noexcept {
 if(!valid_button(button_value,2U)) return false;
 set_bit(state_.buttons,static_cast<unsigned>(button_value),pressed); return true;
}
bool NunchukController::button(NunchukButton button_value) const noexcept {
 return valid_button(button_value,2U)&&(state_.buttons&(1U<<static_cast<unsigned>(button_value)))!=0U;
}
void NunchukController::set_stick(float x,float y) noexcept {
 state_.stick_x=clamp_axis(x); state_.stick_y=clamp_axis(y);
}
void NunchukController::set_acceleration(float x,float y,float z) noexcept {
 state_.acceleration={clamp_axis(x),clamp_axis(y),clamp_axis(z)};
}
bool ClassicController::set_button(ClassicButton button_value,bool pressed) noexcept {
 if(!valid_button(button_value,15U)) return false;
 set_bit(state_.buttons,static_cast<unsigned>(button_value),pressed); return true;
}
bool ClassicController::button(ClassicButton button_value) const noexcept {
 return valid_button(button_value,15U)&&(state_.buttons&(1U<<static_cast<unsigned>(button_value)))!=0U;
}
void ClassicController::set_sticks(float lx,float ly,float rx,float ry) noexcept {
 state_.left_x=clamp_axis(lx); state_.left_y=clamp_axis(ly);
 state_.right_x=clamp_axis(rx); state_.right_y=clamp_axis(ry);
}
void ClassicController::set_triggers(float left,float right) noexcept {
 state_.trigger_l=clamp_trigger(left); state_.trigger_r=clamp_trigger(right);
}
} // namespace xavrx::system
