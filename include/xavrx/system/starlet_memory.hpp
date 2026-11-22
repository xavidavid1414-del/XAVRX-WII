#pragma once
#include "xavrx/memory/memory.hpp"
#include "xavrx/system/wii_memory.hpp"
#include <array>
#include <cstdint>
namespace xavrx::system {
enum class StarletMemoryCpu : std::uint8_t { Broadway, Starlet };
class StarletMemory final : public memory::MemoryMappedDevice {
public:
 static constexpr std::uint32_t sram_base=0x0d400000U;
 static constexpr std::uint32_t mirror_base=0xfff00000U;
 static constexpr std::uint32_t mirror_size=0x00100000U;
 static constexpr std::uint32_t sram_protection_register=0x60U;
 static constexpr std::uint32_t boot0_register=0x18cU;
 static constexpr std::uint32_t sram_a_size=0x10000U;
 static constexpr std::uint32_t sram_b_size=0x8000U;
 static constexpr std::uint32_t boot_rom_size=0x2000U;
 class CpuView final : public memory::MemoryMappedDevice {
 public:
  CpuView(StarletMemory& owner,StarletMemoryCpu cpu,std::uint32_t base) noexcept:owner_(owner),cpu_(cpu),base_(base){}
  bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
  bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 private:
  StarletMemory& owner_; StarletMemoryCpu cpu_; std::uint32_t base_;
 };
 StarletMemory() noexcept;
 [[nodiscard]] bool map_into(WiiMemory& bus) noexcept;
 [[nodiscard]] bool load_boot_rom(const std::array<std::uint8_t,boot_rom_size>& image) noexcept;
 void set_srnprot(std::uint32_t value) noexcept { srnprot_=value; }
 void set_boot0(std::uint32_t value) noexcept { boot0_=value; }
 [[nodiscard]] std::uint32_t srnprot() const noexcept{return srnprot_;}
 [[nodiscard]] std::uint32_t boot0() const noexcept{return boot0_;}
 [[nodiscard]] bool boot_rom_loaded() const noexcept{return boot_rom_loaded_;}
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 [[nodiscard]] bool read_as(StarletMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t& value) noexcept;
 [[nodiscard]] bool write_as(StarletMemoryCpu cpu,std::uint32_t address,std::uint8_t width,std::uint64_t value) noexcept;
 void reset() noexcept;
private:
 enum class Region : std::uint8_t { None, SramA, SramB, BootRom };
 struct Translation {Region region{Region::None};std::uint32_t offset{0};};
 [[nodiscard]] Translation translate(std::uint32_t address) const noexcept;
 [[nodiscard]] bool read_region(Translation mapping,std::uint8_t width,std::uint64_t& value) const noexcept;
 [[nodiscard]] bool write_region(Translation mapping,std::uint8_t width,std::uint64_t value) noexcept;
 std::array<std::uint8_t,sram_a_size> sram_a_{};
 std::array<std::uint8_t,sram_b_size> sram_b_{};
 std::array<std::uint8_t,boot_rom_size> boot_rom_{};
 bool boot_rom_loaded_{false};
 std::uint32_t srnprot_{0},boot0_{0};
 CpuView broadway_sram_{*this,StarletMemoryCpu::Broadway,sram_base};
 CpuView starlet_sram_{*this,StarletMemoryCpu::Starlet,sram_base};
 CpuView broadway_mirror_{*this,StarletMemoryCpu::Broadway,mirror_base};
 CpuView starlet_mirror_{*this,StarletMemoryCpu::Starlet,mirror_base};
};
}
