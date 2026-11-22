#pragma once
#include "xavrx/memory/memory.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <unordered_map>
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
class NandInterface final : public RegisterWindow {
public:
 static constexpr std::size_t page_size=2048,spare_size=64,pages_per_block=64,block_count=4096;
 NandInterface() noexcept;
 [[nodiscard]] bool read_page(std::uint32_t page,std::span<std::uint8_t> data,std::span<std::uint8_t> spare) const noexcept;
 [[nodiscard]] bool program_page(std::uint32_t page,std::span<const std::uint8_t> data,std::span<const std::uint8_t> spare);
 [[nodiscard]] bool erase_block(std::uint32_t block) noexcept;
 [[nodiscard]] std::size_t allocated_pages() const noexcept{return pages_.size();}
 void reset() noexcept{RegisterWindow::reset();}
private:
 using Page=std::array<std::uint8_t,page_size+spare_size>;
 std::unordered_map<std::uint32_t,Page> pages_;
};
class SdHostController final : public RegisterWindow {
public:
 static constexpr std::size_t block_size=512;
 SdHostController() noexcept;
 [[nodiscard]] bool attach_card(std::vector<std::uint8_t> image);
 void detach_card() noexcept;
 [[nodiscard]] bool card_present() const noexcept{return !card_.empty();}
 [[nodiscard]] std::uint32_t block_count() const noexcept{return static_cast<std::uint32_t>(card_.size()/block_size);}
 [[nodiscard]] bool read_blocks(std::uint32_t first,std::uint32_t count,std::span<std::uint8_t> output) const noexcept;
 [[nodiscard]] bool write_blocks(std::uint32_t first,std::uint32_t count,std::span<const std::uint8_t> input) noexcept;
 void reset() noexcept{RegisterWindow::reset();}
private:
 std::vector<std::uint8_t> card_;
};
class UsbHostController final : public RegisterWindow {
public:
 static constexpr std::size_t transfer_capacity=64;
 struct Transfer {std::uint32_t token{0};std::vector<std::uint8_t> payload;};
 UsbHostController() noexcept;
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 void connect_device(bool connected) noexcept;
 [[nodiscard]] bool enqueue_transfer(std::uint32_t token,std::span<const std::uint8_t> payload);
 void advance_microframes(std::uint32_t count) noexcept;
 [[nodiscard]] bool take_completed_transfer(Transfer& transfer);
 [[nodiscard]] std::size_t pending_transfers() const noexcept{return pending_.size();}
 [[nodiscard]] std::size_t completed_transfers() const noexcept{return completed_.size();}
 void reset() noexcept;
private:
 static constexpr std::uint32_t cmd_run=1U,cmd_reset=1U<<1U;
 static constexpr std::uint32_t sts_halted=1U<<12U,sts_usbint=1U<<0U;
 std::uint32_t command_{0},status_{sts_halted},interrupt_enable_{0},frame_index_{0},periodic_base_{0},async_list_{0},config_flag_{0},port_status_{0};
 std::deque<Transfer> pending_,completed_;
};
class DriveInterface final : public RegisterWindow {
public:
 DriveInterface() noexcept;
 [[nodiscard]] bool mount_disc_image(std::vector<std::uint8_t> image);
 void eject_disc() noexcept;
 [[nodiscard]] bool disc_present() const noexcept{return !disc_image_.empty();}
 [[nodiscard]] std::size_t disc_size() const noexcept{return disc_image_.size();}
 [[nodiscard]] bool read_disc_bytes(std::uint64_t offset,std::span<std::uint8_t> output) const noexcept;
private:
 std::vector<std::uint8_t> disc_image_;
};
class WirelessLanController final : public RegisterWindow {
public:
 static constexpr std::size_t function_count=2U,function_address_space=0x20000U;
 WirelessLanController() noexcept;
 [[nodiscard]] bool read_sdio_byte(std::uint8_t function,std::uint32_t address,std::uint8_t& value) const noexcept;
 [[nodiscard]] bool write_sdio_byte(std::uint8_t function,std::uint32_t address,std::uint8_t value) noexcept;
 [[nodiscard]] bool read_sdio_data(std::uint8_t function,std::uint32_t address,std::span<std::uint8_t> output) const noexcept;
 [[nodiscard]] bool write_sdio_data(std::uint8_t function,std::uint32_t address,std::span<const std::uint8_t> input) noexcept;
 void reset() noexcept;
private:
 std::array<std::array<std::uint8_t,function_address_space>,function_count> functions_{};
};
class BluetoothHciTransport final {
public:
 static constexpr std::size_t queue_capacity=64;
 [[nodiscard]] bool process_host_command(std::span<const std::uint8_t> command,std::vector<std::uint8_t>& event);
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
