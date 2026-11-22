#pragma once
#include "xavrx/memory/memory.hpp"
#include <cstddef>
#include <cstdint>
#include <cstddef>
#include <vector>
namespace xavrx::system {
enum class WiiMemoryCpu : std::uint8_t { Broadway, Starlet };
struct DualDeviceMapping { std::uint32_t base; std::uint32_t size; memory::MemoryMappedDevice* broadway; memory::MemoryMappedDevice* starlet; };
class WiiMemory final {
public:
 static constexpr std::uint32_t mem1_cached=0x80000000U,mem1_uncached=0xC0000000U,mem1_physical=0x00000000U;
 static constexpr std::uint32_t mem2_cached=0x90000000U,mem2_uncached=0xD0000000U,mem2_physical=0x10000000U;
 static constexpr std::size_t mem1_size=24U*1024U*1024U,mem2_size=64U*1024U*1024U;
 WiiMemory();
 [[nodiscard]] bool map_device_dual(std::uint32_t base,std::uint32_t size,memory::MemoryMappedDevice& broadway,memory::MemoryMappedDevice& starlet) noexcept;
 [[nodiscard]] bool map_devices_dual(const DualDeviceMapping* mappings,std::size_t count) noexcept;
 [[nodiscard]] memory::AccessResult read(std::uint32_t address,std::uint8_t width,std::uint64_t& value) noexcept;
 [[nodiscard]] memory::AccessResult write(std::uint32_t address,std::uint8_t width,std::uint64_t value) noexcept;
 [[nodiscard]] memory::AccessResult read_as(WiiMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t& value) noexcept;
 [[nodiscard]] memory::AccessResult write_as(WiiMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t value) noexcept;
 [[nodiscard]] memory::AccessResult read8(std::uint32_t address,std::uint8_t& value) noexcept;
 [[nodiscard]] memory::AccessResult read16(std::uint32_t address,std::uint16_t& value) noexcept;
 [[nodiscard]] memory::AccessResult read32(std::uint32_t address,std::uint32_t& value) noexcept;
 [[nodiscard]] memory::AccessResult read64(std::uint32_t address,std::uint64_t& value) noexcept;
 [[nodiscard]] memory::AccessResult write8(std::uint32_t address,std::uint8_t value) noexcept;
 [[nodiscard]] memory::AccessResult write16(std::uint32_t address,std::uint16_t value) noexcept;
 [[nodiscard]] memory::AccessResult write32(std::uint32_t address,std::uint32_t value) noexcept;
 [[nodiscard]] memory::AccessResult write64(std::uint32_t address,std::uint64_t value) noexcept;
 [[nodiscard]] std::size_t mem1_capacity() const noexcept{return mem1_.size();}
 [[nodiscard]] std::size_t mem2_capacity() const noexcept{return mem2_.size();}
 void reset() noexcept;
private:
 using DeviceRegion = DualDeviceMapping;
 [[nodiscard]] bool translate(std::uint32_t address,std::uint8_t width,std::vector<std::uint8_t>*& bank,std::size_t& offset) noexcept;
 std::vector<std::uint8_t> mem1_,mem2_;
 std::vector<DeviceRegion> devices_;
};
}
