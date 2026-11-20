#include "xavrx/memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>

int main() {
    xavrx::Memory memory(64);

    memory.write8(0, 0xAB);
    assert(memory.read8(0) == 0xAB);

    memory.write16_be(2, 0x1234);
    assert(memory.read16_be(2) == 0x1234);
    assert(memory.read8(2) == 0x12);
    assert(memory.read8(3) == 0x34);

    memory.write32_be(8, 0x12345678);
    assert(memory.read32_be(8) == 0x12345678);
    assert(memory.read8(8) == 0x12);
    assert(memory.read8(11) == 0x78);

    assert(memory.contains(60, 4));
    assert(!memory.contains(61, 4));

    bool threw = false;
    try {
        (void)memory.read32_be(62);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    assert(threw);

    std::cout << "All memory tests passed.\n";
    return 0;
}
