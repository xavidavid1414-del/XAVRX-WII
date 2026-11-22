#include "xavrx/system/input_mapping.hpp"
#include <algorithm>
#include <cmath>

namespace xavrx::system {
namespace {
constexpr std::size_t control_count=static_cast<std::size_t>(InputControl::Count);
constexpr std::size_t action_count=static_cast<std::size_t>(GameAction::Count);
template<class E> constexpr bool valid(E value,std::size_t count) noexcept {
 return static_cast<std::size_t>(value)<count;
}
float clean(float value) noexcept { return std::isfinite(value)?std::clamp(value,-1.0F,1.0F):0.0F; }
void button(ControllerInputFrame& frame,InputControl control,bool pressed) noexcept { frame.set(control,pressed?1.0F:0.0F); }
void classic_buttons(ControllerInputFrame& f,std::uint16_t bits) noexcept {
 const auto set=[&](ClassicButton b,InputControl c){button(f,c,(bits&(1U<<static_cast<unsigned>(b)))!=0U);};
 set(ClassicButton::Up,InputControl::ClassicUp); set(ClassicButton::Down,InputControl::ClassicDown);
 set(ClassicButton::Left,InputControl::ClassicLeft); set(ClassicButton::Right,InputControl::ClassicRight);
 set(ClassicButton::A,InputControl::ClassicA); set(ClassicButton::B,InputControl::ClassicB);
 set(ClassicButton::X,InputControl::ClassicX); set(ClassicButton::Y,InputControl::ClassicY);
 set(ClassicButton::L,InputControl::ClassicL); set(ClassicButton::R,InputControl::ClassicR);
 set(ClassicButton::ZL,InputControl::ClassicZL); set(ClassicButton::ZR,InputControl::ClassicZR);
 set(ClassicButton::Plus,InputControl::ClassicPlus); set(ClassicButton::Minus,InputControl::ClassicMinus);
 set(ClassicButton::Home,InputControl::ClassicHome);
}
}
float ControllerInputFrame::get(InputControl control) const noexcept {
 return valid(control,control_count)?values[static_cast<std::size_t>(control)]:0.0F;
}
void ControllerInputFrame::set(InputControl control,float value) noexcept {
 if(valid(control,control_count)) values[static_cast<std::size_t>(control)]=clean(value);
}
bool GameInputState::pressed(GameAction action) const noexcept {
 return valid(action,action_count)&&buttons[static_cast<std::size_t>(action)];
}
float GameInputState::value(GameAction action) const noexcept {
 return valid(action,action_count)?axes[static_cast<std::size_t>(action)]:0.0F;
}
ControllerInputFrame make_input_frame(const WiiRemoteState& s) noexcept {
 ControllerInputFrame f; f.controller=ControllerType::WiiRemote;
 const InputControl controls[]={InputControl::RemoteLeft,InputControl::RemoteRight,InputControl::RemoteDown,InputControl::RemoteUp,
  InputControl::RemotePlus,InputControl::RemoteTwo,InputControl::RemoteOne,InputControl::RemoteB,InputControl::RemoteA,
  InputControl::RemoteMinus,InputControl::RemoteHome};
 for(unsigned i=0;i<11U;++i) button(f,controls[i],(s.buttons&(1U<<i))!=0U);
 f.set(InputControl::PointerX,s.pointer.x*2.0F-1.0F); f.set(InputControl::PointerY,s.pointer.y*2.0F-1.0F);
 return f;
}
ControllerInputFrame make_input_frame(const NunchukState& s) noexcept {
 ControllerInputFrame f; f.controller=ControllerType::Nunchuk;
 button(f,InputControl::NunchukC,(s.buttons&1U)!=0U); button(f,InputControl::NunchukZ,(s.buttons&2U)!=0U);
 f.set(InputControl::NunchukStickX,s.stick_x); f.set(InputControl::NunchukStickY,s.stick_y); return f;
}
ControllerInputFrame make_input_frame(const ClassicControllerState& s) noexcept {
 ControllerInputFrame f; f.controller=ControllerType::Classic; classic_buttons(f,s.buttons);
 f.set(InputControl::ClassicLeftX,s.left_x); f.set(InputControl::ClassicLeftY,s.left_y);
 f.set(InputControl::ClassicRightX,s.right_x); f.set(InputControl::ClassicRightY,s.right_y);
 f.set(InputControl::ClassicTriggerL,s.trigger_l); f.set(InputControl::ClassicTriggerR,s.trigger_r); return f;
}
ControllerInputFrame make_input_frame(const ClassicControllerProState& s) noexcept {
 ControllerInputFrame f; f.controller=ControllerType::ClassicPro; classic_buttons(f,s.buttons);
 f.set(InputControl::ProLeftX,s.left_x); f.set(InputControl::ProLeftY,s.left_y);
 f.set(InputControl::ProRightX,s.right_x); f.set(InputControl::ProRightY,s.right_y); return f;
}
ControllerInputFrame make_input_frame(const WiiBalanceBoardState& s) noexcept {
 ControllerInputFrame f; f.controller=ControllerType::BalanceBoard;
 f.set(InputControl::BalanceWeightKg,std::clamp(s.total_weight_kg/150.0F,0.0F,1.0F));
 f.set(InputControl::BalanceCenterX,s.center_x); f.set(InputControl::BalanceCenterY,s.center_y); return f;
}
bool ControllerInputRouter::add_binding(const InputBinding& b) {
 if(!valid(b.controller,5U)||!valid(b.control,control_count)||!valid(b.action,action_count)||
    !std::isfinite(b.scale)||!std::isfinite(b.deadzone)||b.deadzone<0.0F||b.deadzone>=1.0F) return false;
 bindings_.push_back(b); return true;
}
GameInputState ControllerInputRouter::route(const ControllerInputFrame& f) const noexcept {
 GameInputState result;
 for(const auto& b:bindings_) {
  if(b.controller!=f.controller) continue;
  float value=f.get(b.control)*b.scale;
  if(!std::isfinite(value)) value=0.0F;
  value=std::clamp(value,-1.0F,1.0F);
  if(b.digital) result.buttons[static_cast<std::size_t>(b.action)]=result.buttons[static_cast<std::size_t>(b.action)]||(std::fabs(value)>b.deadzone);
  else {
   if(std::fabs(value)<=b.deadzone) value=0.0F;
   else value=std::copysign((std::fabs(value)-b.deadzone)/(1.0F-b.deadzone),value);
   result.axes[static_cast<std::size_t>(b.action)]=value;
  }
 }
 return result;
}
} // namespace xavrx::system
