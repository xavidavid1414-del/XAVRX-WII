#include "xavrx/core/emulator.hpp"

#include <3ds.h>
#include <cstdio>

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, nullptr);

    constexpr auto version = xavrx::Emulator::version();
    std::printf("XAVRX-WII %.*s\n", static_cast<int>(version.size()), version.data());
    std::printf("Nintendo 3DS homebrew build\n\n");

    xavrx::Emulator emulator;
    if (emulator.initialize()) {
        std::printf("Wii hardware foundation initialized.\n");
        std::printf("Emulation is experimental and not yet playable.\n");
    } else {
        std::printf("Hardware initialization failed.\n");
        std::printf("The 3DS memory/runtime port needs further work.\n");
    }

    std::printf("\nPress START to exit.\n");
    while (aptMainLoop()) {
        hidScanInput();
        if ((hidKeysDown() & KEY_START) != 0) {
            break;
        }
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
