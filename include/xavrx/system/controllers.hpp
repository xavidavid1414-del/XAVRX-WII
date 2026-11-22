#pragma once
#include <array>
#include <cstdint>

namespace xavrx::system {

struct Acceleration3 {
 float x{0.0F}, y{0.0F}, z{1.0F};
};

struct PointerState {
 float x{0.5F}, y{0.5F};
 bool visible{false};
};

enum class WiiRemoteButton : std::uint8_t {
 Left, Right, Down, Up, Plus, Two, One, B, A, Minus, Home
};

struct WiiRemoteState {
 std::uint16_t buttons{0};
 Acceleration3 acceleration{};
 PointerState pointer{};
 bool nunchuk_attached{false};
};

class WiiRemoteController final {
public:
 [[nodiscard]] bool set_button(WiiRemoteButton button, bool pressed) noexcept;
 [[nodiscard]] bool button(WiiRemoteButton button) const noexcept;
 void set_pointer(float x, float y, bool visible) noexcept;
 void set_acceleration(float x, float y, float z) noexcept;
 void set_nunchuk_attached(bool attached) noexcept { state_.nunchuk_attached=attached; }
 [[nodiscard]] const WiiRemoteState& state() const noexcept { return state_; }
 void reset() noexcept { state_=WiiRemoteState{}; }
private:
 WiiRemoteState state_{};
};

enum class NunchukButton : std::uint8_t { C, Z };

struct NunchukState {
 std::uint8_t buttons{0};
 float stick_x{0.0F}, stick_y{0.0F};
 Acceleration3 acceleration{};
};

class NunchukController final {
public:
 [[nodiscard]] bool set_button(NunchukButton button, bool pressed) noexcept;
 [[nodiscard]] bool button(NunchukButton button) const noexcept;
 void set_stick(float x, float y) noexcept;
 void set_acceleration(float x, float y, float z) noexcept;
 [[nodiscard]] const NunchukState& state() const noexcept { return state_; }
 void reset() noexcept { state_=NunchukState{}; }
private:
 NunchukState state_{};
};

enum class ClassicButton : std::uint8_t {
 Up, Down, Left, Right, A, B, X, Y, L, R, ZL, ZR, Plus, Minus, Home
};

struct ClassicControllerState {
 std::uint16_t buttons{0};
 float left_x{0.0F}, left_y{0.0F};
 float right_x{0.0F}, right_y{0.0F};
 float trigger_l{0.0F}, trigger_r{0.0F};
};

class ClassicController final {
public:
 [[nodiscard]] bool set_button(ClassicButton button, bool pressed) noexcept;
 [[nodiscard]] bool button(ClassicButton button) const noexcept;
 void set_sticks(float left_x, float left_y, float right_x, float right_y) noexcept;
 void set_triggers(float left, float right) noexcept;
 [[nodiscard]] const ClassicControllerState& state() const noexcept { return state_; }
 void reset() noexcept { state_=ClassicControllerState{}; }
private:
 ClassicControllerState state_{};
};

// The Pro revision uses digital L/R shoulder buttons and keeps the same
// two-stick and face-button layout. It is a distinct profile from the
// pressure-sensitive shoulder triggers represented by ClassicController.
struct ClassicControllerProState {
 std::uint16_t buttons{0};
 float left_x{0.0F}, left_y{0.0F};
 float right_x{0.0F}, right_y{0.0F};
};

class ClassicControllerPro final {
public:
 [[nodiscard]] bool set_button(ClassicButton button, bool pressed) noexcept;
 [[nodiscard]] bool button(ClassicButton button) const noexcept;
 void set_sticks(float left_x, float left_y, float right_x, float right_y) noexcept;
 [[nodiscard]] const ClassicControllerProState& state() const noexcept { return state_; }
 void reset() noexcept { state_=ClassicControllerProState{}; }
private:
 ClassicControllerProState state_{};
};

enum class BalanceBoardCorner : std::uint8_t { TopLeft, TopRight, BottomLeft, BottomRight };

struct WiiBalanceBoardState {
 std::array<float,4> raw_sensor{};
 std::array<float,4> corner_weight_kg{};
 float total_weight_kg{0.0F};
 // Center of pressure in normalized board coordinates [-1,1].
 float center_x{0.0F}, center_y{0.0F};
 bool has_load{false};
};

class WiiBalanceBoard final {
public:
 // Each sensor converts (raw - offset) into kilograms using kg_per_unit.
 [[nodiscard]] bool set_corner_calibration(BalanceBoardCorner corner, float offset,
                                           float kg_per_unit) noexcept;
 void set_sensor_readings(float top_left, float top_right,
                          float bottom_left, float bottom_right) noexcept;
 // Tare makes the current readings the unloaded baseline for all four sensors.
 void tare() noexcept;
 [[nodiscard]] const WiiBalanceBoardState& state() const noexcept { return state_; }
 void reset() noexcept;
private:
 void recalculate() noexcept;
 std::array<float,4> offsets_{};
 std::array<float,4> kg_per_unit_{{1.0F,1.0F,1.0F,1.0F}};
 WiiBalanceBoardState state_{};
};

} // namespace xavrx::system
