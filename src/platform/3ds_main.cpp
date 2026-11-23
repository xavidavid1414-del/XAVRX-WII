#include "xavrx/core/emulator.hpp"
#include <3ds.h>
#include <cstdio>
#include <cstring>

namespace {
constexpr const char* menu[] = {"Start Wii System", "Load Game", "Controllers", "Settings", "About"};
constexpr const char* categories[] = {"Controls", "Interface", "Audio", "Display", "Performance", "Other Settings"};
constexpr const char* controls[] = {"Wii Remote", "Nunchuk", "Classic Controller", "Controller Profiles", "Calibrate Controls"};
constexpr const char* remote[] = {"Button Mapping", "Pointer Sensitivity", "Motion Sensitivity", "Speaker Volume", "Rumble"};
constexpr const char* nunchuk[] = {"Analog Stick Sensitivity", "C Button Mapping", "Z Button Mapping"};
constexpr const char* classic[] = {"Button Mapping", "Left Stick Sensitivity", "Right Stick Sensitivity", "Trigger Sensitivity"};
enum class Panel { Home, Settings, Controls, About, LoadGame, System };
int selected=0, category=0, control=0;
bool settingsOpen=false;
void frame(PrintConsole& top, PrintConsole& bottom) {
 consoleSelect(&top); consoleClear();
 std::printf("XAVRX-WII                                      V1.0.0-alpha.3\n");
 std::printf("============================================================\n");
 std::printf("                    WII EMULATOR\n\n");
 for(int i=0;i<5;i++) std::printf("%s  %s\n", i==selected?">":" ", menu[i]);
 std::printf("\n------------------------------------------------------------\n");
 std::printf("SYSTEM  Experimental Wii hardware model\n");
 std::printf("CPU     Broadway PowerPC core: partial\n");
 std::printf("GAMES   Wii disc loading: not implemented\n");
 std::printf("\nD-PAD: navigate     A: select     START: exit\n");
 consoleSelect(&bottom); consoleClear();
 std::printf("                                           [ SETTINGS ]\n");
 std::printf("                                      Touch the gear area\n");
 std::printf("\n\n");
 if(!settingsOpen) {
  std::printf("                 XAVRX CONTROL PANEL\n\n");
  std::printf("             Select the gear to open settings.\n");
  std::printf("             The lower screen is touch-enabled.\n");
 } else {
  std::printf("                 SETTINGS\n");
  std::printf("-------------------------------\n");
  for(int i=0;i<6;i++) std::printf("%s %s\n", i==category?">":" ", categories[i]);
  std::printf("\nTap a category to open it. B: close\n");
  if(category==0) {
   std::printf("\nCONTROLS\n");
   for(int i=0;i<5;i++) std::printf("%s %s\n", i==control?">":" ", controls[i]);
   std::printf("\nWii Remote / Nunchuk / Classic Controller\n");
   std::printf("Profiles and calibration are listed here.\n");
  } else {
   std::printf("\n%s\n", categories[category]);
   std::printf("Options will become active as their\n");
   std::printf("corresponding subsystem is implemented.\n");
  }
 }
}
void drawControllers(PrintConsole& top, PrintConsole& bottom) {
 consoleSelect(&top); consoleClear();
 std::printf("XAVRX-WII                                      V1.0.0-alpha.3\\n");
 std::printf("============================================================\\n");
 std::printf("                    CONTROLLERS\\n\\n");
 for(int i=0;i<5;i++) std::printf("%s  %s\\n",i==control?">":" ",controls[i]);
 std::printf("\\nD-PAD: navigate   B: back\\n");
 consoleSelect(&bottom); consoleClear();
 std::printf("                 CONTROL DETAILS\\n");
 std::printf("-------------------------------\\n");
 if(control==0) {
  for(int i=0;i<5;i++) std::printf("%s\\n",remote[i]);
 } else if(control==1) {
  for(int i=0;i<3;i++) std::printf("%s\\n",nunchuk[i]);
 } else if(control==2) {
  for(int i=0;i<4;i++) std::printf("%s\\n",classic[i]);
 } else if(control==3) std::printf("Controller profile storage is not implemented.\\n");
 else std::printf("Calibration is not implemented.\\n");
 std::printf("\\nWii Bluetooth pairing is not implemented.\\n");
}
void drawSimple(PrintConsole& top, PrintConsole& bottom, const char* heading, const char* a, const char* b) {
 consoleSelect(&top); consoleClear(); std::printf("%s\n================================\n%s\n%s\n",heading,a,b);
 consoleSelect(&bottom); consoleClear(); std::printf("B: back");
}
}

int main() {
 gfxInitDefault();
 PrintConsole top{}, bottom{};
 consoleInit(GFX_TOP,&top); consoleInit(GFX_BOTTOM,&bottom);
 xavrx::Emulator emulator; (void)emulator.initialize();
 Panel panel=Panel::Home;
 while(aptMainLoop()) {
  hidScanInput(); const u32 down=hidKeysDown();
  if(down&KEY_START) break;
  if(panel==Panel::Home) {
   if(down&KEY_UP) selected=(selected+4)%5;
   if(down&KEY_DOWN) selected=(selected+1)%5;
   if(down&KEY_A) {
    if(selected==0) panel=Panel::System;
    else if(selected==1) panel=Panel::LoadGame;
    else if(selected==2) panel=Panel::Controllers;
    else if(selected==3) panel=Panel::Settings;
    else if(selected==4) panel=Panel::About;
   }
   touchPosition touch{};
   if(down&KEY_TOUCH) { hidTouchRead(&touch); if(touch.px>=270 && touch.py<=42) panel=Panel::Settings; }
   frame(top,bottom);
  } else if(panel==Panel::Settings) {
   touchPosition touch{};
   if(down&KEY_TOUCH) {
    hidTouchRead(&touch);
    if(touch.px>=270 && touch.py<=42) panel=Panel::Home;
    else if(touch.py>=58 && touch.py<208) {
     int pick=(touch.py-58)/25; if(pick>=0&&pick<6) category=pick;
     else if(category==0) control=(touch.py-58)/25;
    }
   }
   if(down&KEY_B) panel=Panel::Home;
   if(down&KEY_UP) category=(category+5)%6;
   if(down&KEY_DOWN) category=(category+1)%6;
   frame(top,bottom);
  } else {
   if(down&KEY_B) panel=Panel::Home;
   if(panel==Panel::System) drawSimple(top,bottom,"WII SYSTEM STATUS",emulator.initialized()?"Hardware initialization: OK":"Hardware initialization: FAILED","Broadway / Hollywood / Starlet remain partial.");
   if(panel==Panel::LoadGame) drawSimple(top,bottom,"LOAD WII GAME","No Wii ISO/WBFS loader is available yet.","Disc parsing and game execution are not implemented.");
   if(panel==Panel::About) drawSimple(top,bottom,"ABOUT XAVRX-WII","Experimental Wii emulator for Nintendo 3DS.","Build package does not imply Wii game compatibility.");
  }
  gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
 }
 gfxExit(); return 0;
}
