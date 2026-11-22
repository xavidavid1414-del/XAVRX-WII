#pragma once
#include "xavrx/memory/memory.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <vector>
namespace xavrx::system {
class RegisterWindow : public memory::MemoryMappedDevice {
public:
 static constexpr std::size_t word_count=64;
 RegisterWindow(std::uint32_t aperture_size,const std::uint32_t* offsets,std::size_t count,bool little_endian) noexcept;
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 void reset() noexcept;
 [[nodiscard]] std::uint32_t aperture_size() const noexcept{return aperture_size_;}
 [[nodiscard]] bool little_endian() const noexcept{return little_endian_;}
 [[nodiscard]] bool supports(std::uint32_t offset) const noexcept;
private:
 [[nodiscard]] static std::uint32_t byte_swap(std::uint32_t value) noexcept;
 std::uint32_t aperture_size_{0};
 std::array<std::uint32_t,word_count> registers_{};
 std::array<bool,word_count> implemented_{};
 bool little_endian_{false};
};
class NandInterface final : public RegisterWindow { public: NandInterface() noexcept; };
class SdHostController final : public RegisterWindow { public: SdHostController() noexcept; };
class UsbHostController final : public RegisterWindow { public: UsbHostController() noexcept; };
class DriveInterface final : public RegisterWindow { public: DriveInterface() noexcept; };
class WirelessLanController final : public RegisterWindow { public: WirelessLanController() noexcept; };

class BluetoothHciTransport final {
public:
 static constexpr std::size_t queue_capacity=64;
 [[nodiscard]] bool submit_host_packet(std::span<const std::uint8_t> packet);
 [[nodiscard]] bool take_device_packet(std::vector<std::uint8_t>& packet);
 [[nodiscard]] bool submit_device_packet(std::span<const std::uint8_t> packet);
 [[nodiscard]] bool take_host_packet(std::vector<std::uint8_t>& packet);
 void reset() noexcept;
 [[nodiscard]] std::size_t host_queue_size() const noexcept{return host_to_device_.size();}
 [[nodiscard]] std::size_t device_queue_size() const noexcept{return device_to_host_.size();}
private:
 [[nodiscard]] static bool valid_host_packet(std::span<const std::uint8_t> packet) noexcept;
 [[nodiscard]] static bool valid_device_packet(std::span<const std::uint8_t> packet) noexcept;
 std::deque<std::vector<std::uint8_t>> host_to_device_;
 std::deque<std::vector<std::uint8_t>> device_to_host_;
};
}
