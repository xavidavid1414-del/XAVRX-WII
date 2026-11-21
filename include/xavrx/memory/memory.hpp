#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace xavrx::memory {
enum class AccessResult : std::uint8_t { Success, Unmapped, Misaligned, InvalidWidth, DeviceError, InvalidConfiguration };
class MemoryMappedDevice {
public:
 virtual ~MemoryMappedDevice()=default;
 [[nodiscard]] virtual bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept=0;
 [[nodiscard]] virtual bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept=0;
};
class Memory final {
public:
 Memory(std::uint32_t ram_base,std::size_t ram_size);
 [[nodiscard]] bool valid() const noexcept{return valid_;}
 [[nodiscard]] std::uint32_t ram_base() const noexcept{return ram_base_;}
 [[nodiscard]] std::size_t ram_size() const noexcept{return ram_.size();}
 [[nodiscard]] bool map_device(std::uint32_t base,std::uint32_t size,MemoryMappedDevice& device) noexcept;
 [[nodiscard]] AccessResult read(std::uint32_t address,std::uint8_t width,std::uint64_t& value) noexcept;
 [[nodiscard]] AccessResult write(std::uint32_t address,std::uint8_t width,std::uint64_t value) noexcept;
 [[nodiscard]] AccessResult read8(std::uint32_t a,std::uint8_t& v) noexcept;
 [[nodiscard]] AccessResult read16(std::uint32_t a,std::uint16_t& v) noexcept;
 [[nodiscard]] AccessResult read32(std::uint32_t a,std::uint32_t& v) noexcept;
 [[nodiscard]] AccessResult read64(std::uint32_t a,std::uint64_t& v) noexcept;
 [[nodiscard]] AccessResult write8(std::uint32_t a,std::uint8_t v) noexcept;
 [[nodiscard]] AccessResult write16(std::uint32_t a,std::uint16_t v) noexcept;
 [[nodiscard]] AccessResult write32(std::uint32_t a,std::uint32_t v) noexcept;
 [[nodiscard]] AccessResult write64(std::uint32_t a,std::uint64_t v) noexcept;
private:
 struct DeviceRegion {std::uint32_t base;std::uint32_t size;MemoryMappedDevice* device;};
 [[nodiscard]] bool contains(std::uint32_t base,std::uint32_t size,std::uint32_t address,std::uint8_t width) const noexcept;
 std::uint32_t ram_base_{};std::vector<std::uint8_t> ram_{};std::vector<DeviceRegion> devices_{};bool valid_{false};
};
}
