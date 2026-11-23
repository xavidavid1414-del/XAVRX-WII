#include "xavrx/core/emulator.hpp"

#include <3ds.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

enum class Screen { Menu, SystemStatus, Games, Controllers, BalanceBoard, Settings, About, BalanceDemo };

constexpr const char* menuItems[] = {
    "Start Wii system",
    "Games / demos",
    "Controllers",
    "Wii Balance Board",
    "Settings",
    "About"
};
constexpr int menuCount = static_cast<int>(sizeof(menuItems) / sizeof(menuItems[0]));

constexpr const char* controllerNames[] = {
    "Wii Remote",
    "Nunchuk",
    "Classic Controller",
    "Classic Controller Pro",
    "Wii MotionPlus (optional)"
};
constexpr const char* controllerDetails[] = {
    "D-pad, A/B, +/-, HOME, 1/2, motion and IR pointer.",
    "Analog stick, C/Z and accelerometer.",
    "Dual sticks, D-pad, A/B/X/Y, L/R/ZL/ZR, +/- and HOME.",
    "Dual sticks, D-pad, A/B/X/Y, L/R/ZL/ZR, +/- and HOME.",
    "Motion extension for the Wii Remote."
};

void clearScreens(PrintConsole& top, PrintConsole& bottom) {
    consoleSelect(&top);
    consoleClear();
    consoleSelect(&bottom);
    consoleClear();
    consoleSelect(&top);
}

void present() {
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}

void drawMenu(PrintConsole& top, PrintConsole& bottom, int selected,
              const xavrx::Emulator& emulator) {
    clearScreens(top, bottom);
    std::printf("XAVRX-WII  |  HOME MENU\n");
    std::printf("==============================\n");
    std::printf("Nintendo 3DS / 2DS - version %.*s\n\n",
                static_cast<int>(xavrx::Emulator::version().size()),
                xavrx::Emulator::version().data());
    for (int i = 0; i < menuCount; ++i) {
        std::printf("%s %s\n", i == selected ? ">" : " ", menuItems[i]);
    }
    std::printf("\nSystem status: %s\n",
                emulator.initialized() ? "initialized" : "not initialized");
    std::printf("Build: experimental 3DS homebrew\n");

    consoleSelect(&bottom);
    std::printf("CONTROLS\n");
    std::printf("D-pad: move   A: select\n");
    std::printf("B: back        START: exit\n\n");
    std::printf("Note: commercial Wii games do not run yet.\n");
    std::printf("The included demo is a native 3DS minigame.");
}

void drawSystemStatus(PrintConsole& top, PrintConsole& bottom, const xavrx::Emulator& emulator) {
    clearScreens(top, bottom);
    std::printf("WII SYSTEM STATUS\n");
    std::printf("==============================\n");
    std::printf("Initialization: %s\n", emulator.initialized() ? "successful" : "failed");
    std::printf("Memory and devices: experimental model\n");
    std::printf("Broadway CPU: core in development\n");
    std::printf("Starlet CPU: core in development\n");
    std::printf("GX GPU / DSP: in development\n");
    consoleSelect(&bottom);
    std::printf("The base system can initialize, but it does not\n");
    std::printf("boot the original Wii Menu or load games yet.\n\n");
    std::printf("B: back");
}

void drawGames(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("GAMES AND DEMOS\n");
    std::printf("==============================\n");
    std::printf("> XAVRX Balance Demo\n");
    std::printf("  Status: ready to play\n");
    std::printf("  Type: native 3DS minigame\n");
    std::printf("\nA: start demo   B: back\n");
    consoleSelect(&bottom);
    std::printf("IMPORTANT\n");
    std::printf("This demo tests the menu and analog input\n");
    std::printf("and a four-sensor visual meter. It is not Wii Fit\n");
    std::printf("and cannot run Wii game files.");
}

void drawControllers(PrintConsole& top, PrintConsole& bottom, int selected) {
    clearScreens(top, bottom);
    std::printf("CONTROLLERS AND PERIPHERALS\n");
    std::printf("==============================\n");
    for (int i = 0; i < 5; ++i) {
        std::printf("%s %s\n", i == selected ? ">" : " ", controllerNames[i]);
    }
    consoleSelect(&bottom);
    std::printf("%s\n\n", controllerDetails[selected]);
    std::printf("Selected profile: %s\n", controllerNames[selected]);
    std::printf("A: change profile   B: back\n");
    std::printf("\nThese profiles describe planned mappings; they do not\n");
    std::printf("mean Wii controllers are connected over Bluetooth.");
}

void drawBalanceBoard(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("WII BALANCE BOARD\n");
    std::printf("==============================\n");
    std::printf("Sensors: 4 load cells\n");
    std::printf("  Front left  [ FL ]\n");
    std::printf("  Front right    [ FR ]\n");
    std::printf("  Rear left    [ RL ]\n");
    std::printf("  Rear right      [ RR ]\n");
    std::printf("\nMeter: available in DEMO mode\n");
    consoleSelect(&bottom);
    std::printf("Physical connection: unavailable in this version.\n");
    std::printf("The 2DS XL has no weight sensors or\n");
    std::printf("a compatible Balance Board receiver.\n\n");
    std::printf("A: open interactive demo\n");
    std::printf("B: back");
}

void drawSettings(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("SETTINGS\n");
    std::printf("==============================\n");
    std::printf("Platform: Nintendo 3DS / 2DS\n");
    std::printf("Display: dual screens\n");
    std::printf("Local input: buttons + Circle Pad\n");
    std::printf("Wii audio: in development\n");
    std::printf("GX graphics: in development\n");
    std::printf("Wii Bluetooth: not implemented\n");
    consoleSelect(&bottom);
    std::printf("Advanced settings will be added as\n");
    std::printf("emulator subsystems are implemented.\n\n");
    std::printf("B: back");
}

void drawAbout(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("ABOUT XAVRX-WII\n");
    std::printf("==============================\n");
    std::printf("Project: experimental Wii emulator\n");
    std::printf("Version: %.*s\n",
                static_cast<int>(xavrx::Emulator::version().size()),
                xavrx::Emulator::version().data());
    std::printf("Homebrew format: .3dsx\n");
    std::printf("Goal: Wii hardware modeling\n");
    consoleSelect(&bottom);
    std::printf("The .3dsx build confirms the app was\n");
    std::printf("built for 3DS; it does not prove compatibility\n");
    std::printf("with commercial games or Wii-level performance.\n\n");
    std::printf("B: back");
}

void drawBalanceDemo(PrintConsole& top, PrintConsole& bottom,
                     float& x, float& y, unsigned int stableFrames) {
    circlePosition circle{};
    hidCircleRead(&circle);
    x = std::clamp(static_cast<float>(circle.dx) / 156.0f, -1.0f, 1.0f);
    y = std::clamp(-static_cast<float>(circle.dy) / 156.0f, -1.0f, 1.0f);

    constexpr float quarterLoad = 12.5f;
    const float fl = quarterLoad * (1.0f - x) * (1.0f - y);
    const float fr = quarterLoad * (1.0f + x) * (1.0f - y);
    const float rl = quarterLoad * (1.0f - x) * (1.0f + y);
    const float rr = quarterLoad * (1.0f + x) * (1.0f + y);

    clearScreens(top, bottom);
    std::printf("XAVRX BALANCE DEMO\n");
    std::printf("==============================\n");
    std::printf("MEDIDOR SIMULADO - NO ES PESO REAL\n\n");
    std::printf("Carga total ficticia: 50.0 kg\n");
    std::printf("FL: %5.1f kg     FR: %5.1f kg\n", fl, fr);
    std::printf("RL: %5.1f kg     RR: %5.1f kg\n", rl, rr);
    std::printf("\nCentro de presion simulado\n");
    std::printf("X: %+0.2f     Y: %+0.2f\n", x, y);
    std::printf("\n");
    std::printf("              ADELANTE\n");
    std::printf("        [ FL ]       [ FR ]\n");
    std::printf("              +\n");
    std::printf("        [ RL ]       [ RR ]\n");
    std::printf("               ATRAS\n");

    consoleSelect(&bottom);
    std::printf("Move the Circle Pad to shift the simulated load.\n");
    std::printf("Stable time near center: %u frames\n", stableFrames);
    std::printf("\nB: end demo and return");
    std::printf("\nAll four values are calculated in software;\n");
    std::printf("they do not come from physical sensors.");
}

} // namespace

int main() {
    gfxInitDefault();
    PrintConsole topConsole{};
    PrintConsole bottomConsole{};
    consoleInit(GFX_TOP, &topConsole);
    consoleInit(GFX_BOTTOM, &bottomConsole);

    xavrx::Emulator emulator;
    (void)emulator.initialize();

    int selected = 0;
    int controllerSelected = 0;
    Screen screen = Screen::Menu;
    float demoX = 0.0f;
    float demoY = 0.0f;
    unsigned int stableFrames = 0;

    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();

        if ((down & KEY_START) != 0) {
            break;
        }

        switch (screen) {
        case Screen::Menu:
            if ((down & KEY_UP) != 0) selected = (selected + menuCount - 1) % menuCount;
            if ((down & KEY_DOWN) != 0) selected = (selected + 1) % menuCount;
            if ((down & KEY_A) != 0) {
                switch (selected) {
                case 0: screen = Screen::SystemStatus; break;
                case 1: screen = Screen::Games; break;
                case 2: screen = Screen::Controllers; break;
                case 3: screen = Screen::BalanceBoard; break;
                case 4: screen = Screen::Settings; break;
                case 5: screen = Screen::About; break;
                default: break;
                }
            }
            drawMenu(topConsole, bottomConsole, selected, emulator);
            break;

        case Screen::SystemStatus:
            if ((down & KEY_B) != 0) screen = Screen::Menu;
            if (screen == Screen::SystemStatus) drawSystemStatus(topConsole, bottomConsole, emulator);
            break;

        case Screen::Games:
            if ((down & KEY_A) != 0) {
                screen = Screen::BalanceDemo;
                stableFrames = 0;
            } else if ((down & KEY_B) != 0) {
                screen = Screen::Menu;
            }
            if (screen == Screen::Games) drawGames(topConsole, bottomConsole);
            break;

        case Screen::Controllers:
            if ((down & KEY_UP) != 0) controllerSelected = (controllerSelected + 4) % 5;
            if ((down & KEY_DOWN) != 0) controllerSelected = (controllerSelected + 1) % 5;
            if ((down & KEY_A) != 0) controllerSelected = (controllerSelected + 1) % 5;
            if ((down & KEY_B) != 0) screen = Screen::Menu;
            if (screen == Screen::Controllers) drawControllers(topConsole, bottomConsole, controllerSelected);
            break;

        case Screen::BalanceBoard:
            if ((down & KEY_A) != 0) {
                screen = Screen::BalanceDemo;
                stableFrames = 0;
            } else if ((down & KEY_B) != 0) {
                screen = Screen::Menu;
            }
            if (screen == Screen::BalanceBoard) drawBalanceBoard(topConsole, bottomConsole);
            break;

        case Screen::Settings:
            if ((down & KEY_B) != 0) screen = Screen::Menu;
            if (screen == Screen::Settings) drawSettings(topConsole, bottomConsole);
            break;

        case Screen::About:
            if ((down & KEY_B) != 0) screen = Screen::Menu;
            if (screen == Screen::About) drawAbout(topConsole, bottomConsole);
            break;

        case Screen::BalanceDemo: {
            circlePosition circle{};
            hidCircleRead(&circle);
            const float nx = static_cast<float>(circle.dx) / 156.0f;
            const float ny = static_cast<float>(circle.dy) / 156.0f;
            if (std::abs(nx) < 0.15f && std::abs(ny) < 0.15f) ++stableFrames;
            else stableFrames = 0;
            if ((down & KEY_B) != 0) {
                screen = Screen::Games;
                break;
            }
            drawBalanceDemo(topConsole, bottomConsole, demoX, demoY, stableFrames);
            break;
        }
        }

        present();
    }

    gfxExit();
    return 0;
}
