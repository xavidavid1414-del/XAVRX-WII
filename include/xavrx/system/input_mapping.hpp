#pragma once
#include "xavrx/system/controllers.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace xavrx::system {

enum class ControllerType : std::uint8_t { WiiRemote, Nunchuk, Classic, ClassicPro, BalanceBoard };
enum class InputControl : std::uint8_t {
 RemoteLeft, RemoteRight, RemoteDown, RemoteUp, RemotePlus, RemoteTwo, RemoteOne, RemoteB, RemoteA, RemoteMinus, RemoteHome,
 NunchukC, NunchukZ, NunchukStickX, NunchukStickY,
 ClassicUp, ClassicDown, ClassicLeft, ClassicRight, ClassicA, ClassicB, ClassicX, ClassicY,
 ClassicL, ClassicR, ClassicZL, ClassicZR, ClassicPlus, ClassicMinus, ClassicHome,
 ClassicLeftX, ClassicLeftY, ClassicRightX, ClassicRightY, ClassicTriggerL, ClassicTriggerR,
 ProLeftX, ProLeftY, ProRightX, ProRightY,
 PointerX, PointerY, BalanceWeightKg, BalanceCenterX, BalanceCenterY,
 Count
};
enum class GameAction : std::uint8_t {
 Confirm, Cancel, Menu, Pause, Up, Down, Left, Right, Primary, Secondary,
 MoveX, MoveY, CameraX, CameraY, PointerX, PointerY, WeightKg, BalanceX, BalanceY,
 Count
};

struct ControllerInputFrame {
 ControllerType controller{ControllerType::WiiRemote};
 std::array<float,static_cast<std::size_t>(InputControl::Count)> values{};
 [[nodiscard]] float get(InputControl control) const noexcept;
 void set(InputControl control,float value) noexcept;
};

struct InputBinding {
 ControllerType controller{ControllerType::WiiRemote};
 InputControl control{InputControl::RemoteA};
 GameAction action{GameAction::Confirm};
 float scale{1.0F};
 float deadzone{0.15F};
 bool digital{true};
};

struct GameInputState {
 std::array<bool,static_cast<std::size_t>(GameAction::Count)> buttons{};
 std::array<float,static_cast<std::size_t>(GameAction::Count)> axes{};
 [[nodiscard]] bool pressed(GameAction action) const noexcept;
 [[nodiscard]] float value(GameAction action) const noexcept;
 void clear() noexcept { buttons={}; axes={}; }
};

[[nodiscard]] ControllerInputFrame make_input_frame(const WiiRemoteState& state) noexcept;
[[nodiscard]] ControllerInputFrame make_input_frame(const NunchukState& state) noexcept;
[[nodiscard]] ControllerInputFrame make_input_frame(const ClassicControllerState& state) noexcept;
[[nodiscard]] ControllerInputFrame make_input_frame(const ClassicControllerProState& state) noexcept;
[[nodiscard]] ControllerInputFrame make_input_frame(const WiiBalanceBoardState& state) noexcept;

class ControllerInputRouter final {
public:
 [[nodiscard]] bool add_binding(const InputBinding& binding);
 void clear_bindings() noexcept { bindings_.clear(); }
 [[nodiscard]] GameInputState route(const ControllerInputFrame& frame) const noexcept;
 [[nodiscard]] const std::vector<InputBinding>& bindings() const noexcept { return bindings_; }
private:
 std::vector<InputBinding> bindings_{};
};

} // namespace xavrx::system
