#include "xavrx/core/emulator.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc > 1 && std::string_view{argv[1]} == "--version") {
        std::cout << "XAVRX-WII " << xavrx::Emulator::version() << '\n';
        return 0;
    }

    std::cout << "XAVRX-WII " << xavrx::Emulator::version() << '\n'
              << "Nintendo Wii emulator foundation. Emulation core is not implemented yet.\n"
              << "Use --version to display version information.\n";
    return 0;
}
