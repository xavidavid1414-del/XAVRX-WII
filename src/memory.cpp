#include "xavrx/memory.hpp"

#include <stdexcept>

namespace xavrx {

Memory::Memory(std::size_t size) : bytes_(size, 0) {}

std::size_t Memory::size() const noexcept {
    return bytes_.size();
}

bool Memory::contains(std::uint32_t address, std::size_t length) const noexcept {
    const auto start = static_cast<std::size_t>(address);
    return start <= bytes_.size() && length <= bytes_.size() - start;
}

void Memory::check_range(std::uint32_t address, std::size_t length) const {
    if (!contains(address, length)) {
        throw std::out_of_range("XAVRX-WII: memory access outside allocated RAM");
    }
}

std::uint8_t Memory::read8(std::uint32_t address) const {
    check_range(address, 1);
    return bytes_[address];
}

std::uint16_t Memory::read16_be(std::uint32_t address) const {
    check_range(address, 2);
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes_[address]) << 8) |
        static_cast<std::uint16_t>(bytes_[address + 1]));
}

std::uint32_t Memory::read32_be(std::uint32_t address) const {
    check_range(address, 4);
    return (static_cast<std::uint32_t>(bytes_[address]) << 24) |
           (static_cast<std::uint32_t>(bytes_[address + 1]) << 16) |
           (static_cast<std::uint32_t>(bytes_[address + 2]) << 8) |
           static_cast<std::uint32_t>(bytes_[address + 3]);
}

void Memory::write8(std::uint32_t address, std::uint8_t value) {
    check_range(address, 1);
    bytes_[address] = value;
}

void Memory::write16_be(std::uint32_t address, std::uint16_t value) {
    check_range(address, 2);
    bytes_[address] = static_cast<std::uint8_t>(value >> 8);
    bytes_[address + 1] = static_cast<std::uint8_t>(value);
}

void Memory::write32_be(std::uint32_t address, std::uint32_t value) {
    check_range(address, 4);
    bytes_[address] = static_cast<std::uint8_t>(value >> 24);
    bytes_[address + 1] = static_cast<std::uint8_t>(value >> 16);
    bytes_[address + 2] = static_cast<std::uint8_t>(value >> 8);
    bytes_[address + 3] = static_cast<std::uint8_t>(value);
}

} // namespace xavrx
