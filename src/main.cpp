#include "xavrx/memory.hpp"

#include <iostream>

int main() {
    xavrx::Memory ram(1024 * 1024);
    std::cout << "XAVRX-WII experimental emulator\n";
    std::cout << "Core scaffold initialized. RAM: " << ram.size() << " bytes\n";
    std::cout << "No Wii games are executable yet.\n";
    return 0;
}
