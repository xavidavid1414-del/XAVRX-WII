#include "xavrx/core/emulator.hpp"

#include <iostream>
#include <string_view>

int main() {
    const xavrx::Emulator emulator;

    if (xavrx::Emulator::version() != std::string_view{"0.1.0"}) {
        std::cerr << "Version smoke test failed\n";
        return 1;
    }

    if (emulator.initialized()) {
        std::cerr << "Fresh emulator must not report initialized hardware\n";
        return 1;
    }

    std::cout << "All XAVRX-WII foundation tests passed\n";
    return 0;
}
