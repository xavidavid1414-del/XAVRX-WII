#include "xavrx/core/emulator.hpp"

#include <3ds.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

enum class Screen { Menu, SystemStatus, Games, Controllers, BalanceBoard, Settings, About, BalanceDemo };

constexpr const char* menuItems[] = {
    "Iniciar sistema Wii",
    "Juegos / demos",
    "Mandos y controles",
    "Wii Balance Board",
    "Configuracion",
    "Acerca de"
};
constexpr int menuCount = static_cast<int>(sizeof(menuItems) / sizeof(menuItems[0]));

constexpr const char* controllerNames[] = {
    "Wii Remote",
    "Nunchuk",
    "Classic Controller",
    "Classic Controller Pro",
    "Wii MotionPlus (opcional)"
};
constexpr const char* controllerDetails[] = {
    "Cruceta, A/B, +/-, HOME, 1/2, movimiento y puntero IR.",
    "Stick analogico, C/Z y acelerometro.",
    "Dos sticks, cruceta, A/B/X/Y, L/R/ZL/ZR, +/- y HOME.",
    "Dos sticks, cruceta, A/B/X/Y, L/R/ZL/ZR, +/- y HOME.",
    "Extension de movimiento para el Wii Remote."
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
    std::printf("\nEstado del sistema: %s\n",
                emulator.initialized() ? "inicializado" : "no inicializado");
    std::printf("Build: homebrew experimental para 3DS\n");

    consoleSelect(&bottom);
    std::printf("CONTROLES\n");
    std::printf("Cruceta: mover   A: seleccionar\n");
    std::printf("B: volver        START: salir\n\n");
    std::printf("Aviso: aun no ejecuta juegos comerciales de Wii.\n");
    std::printf("La demo incluida es un minijuego nativo de 3DS.");
}

void drawSystemStatus(PrintConsole& top, PrintConsole& bottom, const xavrx::Emulator& emulator) {
    clearScreens(top, bottom);
    std::printf("ESTADO DEL SISTEMA WII\\n");
    std::printf("==============================\\n");
    std::printf("Inicializacion: %s\\n", emulator.initialized() ? "correcta" : "fallida");
    std::printf("Memoria y dispositivos: modelo experimental\\n");
    std::printf("CPU Broadway: nucleo en desarrollo\\n");
    std::printf("CPU Starlet: nucleo en desarrollo\\n");
    std::printf("GPU GX / DSP: en desarrollo\\n");
    consoleSelect(&bottom);
    std::printf("El sistema base puede inicializarse, pero aun no\\n");
    std::printf("arranca el menu original de Wii ni carga juegos.\\n\\n");
    std::printf("B: volver");
}

void drawGames(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("JUEGOS Y DEMOS\n");
    std::printf("==============================\n");
    std::printf("> XAVRX Balance Demo\n");
    std::printf("  Estado: disponible para jugar\n");
    std::printf("  Tipo: minijuego nativo de 3DS\n");
    std::printf("\nA: iniciar demo   B: volver\n");
    consoleSelect(&bottom);
    std::printf("IMPORTANTE\n");
    std::printf("Esta demo permite probar el menu, la entrada analogica\n");
    std::printf("y el medidor visual de cuatro sensores. No es Wii Fit\n");
    std::printf("ni ejecuta un archivo de juego de Wii.");
}

void drawControllers(PrintConsole& top, PrintConsole& bottom, int selected) {
    clearScreens(top, bottom);
    std::printf("MANDOS Y PERIFERICOS\n");
    std::printf("==============================\n");
    for (int i = 0; i < 5; ++i) {
        std::printf("%s %s\n", i == selected ? ">" : " ", controllerNames[i]);
    }
    consoleSelect(&bottom);
    std::printf("%s\n\n", controllerDetails[selected]);
    std::printf("Perfil seleccionado: %s\n", controllerNames[selected]);
    std::printf("A: cambiar perfil   B: volver\n");
    std::printf("\nLos perfiles describen el mapeo previsto; no\n");
    std::printf("significan que haya mandos Wii conectados por Bluetooth.");
}

void drawBalanceBoard(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("WII BALANCE BOARD\n");
    std::printf("==============================\n");
    std::printf("Sensores: 4 celdas de carga\n");
    std::printf("  Delantero izquierdo  [ FL ]\n");
    std::printf("  Delantero derecho    [ FR ]\n");
    std::printf("  Trasero izquierdo    [ RL ]\n");
    std::printf("  Trasero derecho      [ RR ]\n");
    std::printf("\nMedidor: disponible en modo DEMO\n");
    consoleSelect(&bottom);
    std::printf("Conexion fisica: no disponible en esta version.\n");
    std::printf("El 2DS XL no incorpora los sensores de peso ni\n");
    std::printf("un receptor compatible para la Balance Board.\n\n");
    std::printf("A: abrir demo interactiva\n");
    std::printf("B: volver");
}

void drawSettings(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("CONFIGURACION\n");
    std::printf("==============================\n");
    std::printf("Plataforma: Nintendo 3DS / 2DS\n");
    std::printf("Pantalla: doble pantalla\n");
    std::printf("Entrada local: botones + Circle Pad\n");
    std::printf("Audio Wii: en desarrollo\n");
    std::printf("Graficos GX: en desarrollo\n");
    std::printf("Bluetooth Wii: no implementado\n");
    consoleSelect(&bottom);
    std::printf("Los ajustes avanzados se habilitaran conforme\n");
    std::printf("se implementen los subsistemas del emulador.\n\n");
    std::printf("B: volver");
}

void drawAbout(PrintConsole& top, PrintConsole& bottom) {
    clearScreens(top, bottom);
    std::printf("ACERCA DE XAVRX-WII\n");
    std::printf("==============================\n");
    std::printf("Proyecto: emulador experimental de Wii\n");
    std::printf("Version: %.*s\n",
                static_cast<int>(xavrx::Emulator::version().size()),
                xavrx::Emulator::version().data());
    std::printf("Formato homebrew: .3dsx\n");
    std::printf("Objetivo: modelado de hardware Wii\n");
    consoleSelect(&bottom);
    std::printf("La compilacion .3dsx confirma que la aplicacion\n");
    std::printf("se construye para 3DS; no demuestra compatibilidad\n");
    std::printf("con juegos comerciales ni rendimiento de Wii.\n\n");
    std::printf("B: volver");
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
    std::printf("Mueve el Circle Pad para desplazar la carga simulada.\n");
    std::printf("Tiempo estable cerca del centro: %u cuadros\n", stableFrames);
    std::printf("\nB: terminar demo y volver");
    std::printf("\nLos cuatro valores son calculados por software;\n");
    std::printf("no proceden de sensores fisicos.");
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
