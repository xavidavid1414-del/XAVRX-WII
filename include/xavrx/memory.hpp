#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace xavrx {

class Memory {
public:
    explicit Memory(std::size_t size);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool contains(std::uint32_t address, std::size_t length = 1) const noexcept;

    [[nodiscard]] std::uint8_t read8(std::uint32_t address) const;
    [[nodiscard]] std::uint16_t read16_be(std::uint32_t address) const;
    [[nodiscard]] std::uint32_t read32_be(std::uint32_t address) const;

    void write8(std::uint32_t address, std::uint8_t value);
    void write16_be(std::uint32_t address, std::uint16_t value);
    void write32_be(std::uint32_t address, std::uint32_t value);

private:
    void check_range(std::uint32_t address, std::size_t length) const;
    std::vector<std::uint8_t> bytes_;
};

} // namespace xavrx
