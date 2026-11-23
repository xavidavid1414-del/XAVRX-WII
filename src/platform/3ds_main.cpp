#include "xavrx/core/emulator.hpp"
#include <3ds.h>
#include <cstdio>

namespace {
constexpr const char* menu[] = {"[PLAY]  Start Wii System", "[DISC]  Load Game", "[PAD]   Controllers", "[GEAR]  Settings", "[INFO]  About"};
constexpr const char* categories[] = {"Controls", "Interface", "Audio", "Display", "Performance", "Other Settings"};
constexpr const char* controls[] = {"Wii Remote", "Nunchuk", "Classic Controller", "Wii Remote + Nunchuk", "Controller Profiles", "Calibrate Controls"};
constexpr const char* physical[] = {"A", "B", "X", "Y", "L", "R", "ZL", "ZR", "D-Pad", "Circle Pad", "C-Stick", "START", "SELECT", "HOME"};
constexpr const char* remoteNames[] = {"A", "B", "1", "2", "+", "-", "HOME", "D-Pad"};
constexpr const char* nunchukNames[] = {"Analog Stick", "C", "Z"};
constexpr const char* classicNames[] = {"A", "B", "X", "Y", "L", "R", "ZL", "ZR", "Left Stick", "Right Stick", "+ / -", "D-Pad"};
int remoteMap[] = {0, 1, 2, 3, 11, 12, 13, 8};
int nunchukMap[] = {9, 4, 5};
int classicMap[] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 8};
int selected=0, category=0, control=0, mappingRow=0;
bool settingsOpen=false;
int clampMap(int value) { return (value + 14) % 14; }

void drawHome(PrintConsole& top, PrintConsole& bottom) {
 consoleSelect(&top); consoleClear();
 std::printf("XAVRX-WII | alpha.6\n");
 std::printf("\x1b[47;30m\n");
 std::printf("              XAVRX-WII  [WII]\n\n");
 for(int i=0;i<5;i++) std::printf("%s  %s\n", i==selected?" > ":"   ", menu[i]);
 std::printf("\n------------------------------------------------------------\n");
 std::printf("SYSTEM  Experimental Wii hardware model\n");
 std::printf("CPU     Broadway PowerPC core: partial\n");
 std::printf("GAMES   Wii disc loading: not implemented\n");
 std::printf("\nD-PAD: navigate     A: select     START: exit\n");
 consoleSelect(&bottom); consoleClear();
 std::printf("                  [ SETTINGS ]\n");
 std::printf("                                      Touch the gear area\n\n\n");
 if(!settingsOpen) {
  std::printf("                 XAVRX CONTROL PANEL\n\n");
  std::printf("             Select the gear to open settings.\n");
  std::printf("             The lower screen is touch-enabled.\n");
 } else {
  std::printf("                 SETTINGS\n-------------------------------\n");
  for(int i=0;i<6;i++) std::printf("%s %s\n", i==category?">":" ", categories[i]);
  if(category==0) {
   std::printf("\nCONTROLS\n");
   for(int i=0;i<6;i++) std::printf("%s %s\n", i==control?">":" ", controls[i]);
  } else {
   std::printf("\n%s\nOptions activate as their\nsubsystems are implemented.\n", categories[category]);
  }
 }
}

void drawMapRows(const char* const* names, const int* values, int count, int base, int selectedRow) {
 for(int i=0;i<count;i++)
  std::printf("%s %-12s = %s\n", selectedRow==base+i?">":" ", names[i], physical[values[i]]);
}

void drawControllers(PrintConsole& top, PrintConsole& bottom) {
 consoleSelect(&top); consoleClear();
 std::printf("XAVRX-WII | alpha.6\n");
 std::printf("================================================\n");
 std::printf("             NINTENDO 2DS XL CONTROLLER MAP\n\n");
 for(int i=0;i<6;i++) std::printf("%s  %s\n",i==control?">":" ",controls[i]);
 std::printf("\nUP/DOWN: controller  L/R: mapping row\nB: back  A: change selected mapping  X: reset\n");

 consoleSelect(&bottom); consoleClear();
 std::printf("       %s\n--------------------------------\n", controls[control]);
 if(control==0) {
  drawMapRows(remoteNames,remoteMap,8,0,mappingRow);
 } else if(control==1) {
  drawMapRows(nunchukNames,nunchukMap,3,0,mappingRow);
 } else if(control==2) {
  drawMapRows(classicNames,classicMap,12,0,mappingRow);
 } else if(control==3) {
  std::printf("WII REMOTE (independent buttons)\n");
  drawMapRows(remoteNames,remoteMap,8,0,mappingRow);
  std::printf("\nNUNCHUK (independent controls)\n");
  drawMapRows(nunchukNames,nunchukMap,3,8,mappingRow);
  std::printf("\nBoth are one controller set.\nButtons are NOT merged.\n");
 } else if(control==4) {
  std::printf("Controller Profiles\nProfile saving is not implemented.\n");
 } else {
  std::printf("Calibrate Controls\nCalibration is not implemented.\n");
 }
 if(control<=3) {
  std::printf("\nTouch a row or use L/R.\n");
  std::printf("A: cycle 2DS XL assignment\nX: reset this controller\n");
  std::printf("\nEDITOR PREVIEW ONLY\nNot yet connected to game input.\n");
 }
}

void cycleSelectedMapping() {
 if(control==0) remoteMap[mappingRow]=clampMap(remoteMap[mappingRow]+1);
 else if(control==1) nunchukMap[mappingRow]=clampMap(nunchukMap[mappingRow]+1);
 else if(control==2) classicMap[mappingRow]=clampMap(classicMap[mappingRow]+1);
 else if(control==3) {
  if(mappingRow<8) remoteMap[mappingRow]=clampMap(remoteMap[mappingRow]+1);
  else nunchukMap[mappingRow-8]=clampMap(nunchukMap[mappingRow-8]+1);
 }
}
int mappingCount() {
 if(control==0) return 8;
 if(control==1) return 3;
 if(control==2) return 12;
 if(control==3) return 11;
 return 0;
}
void resetMapping() {
 if(control==0) { const int d[]={0,1,2,3,11,12,13,8}; for(int i=0;i<8;i++) remoteMap[i]=d[i]; }
 else if(control==1) { const int d[]={9,4,5}; for(int i=0;i<3;i++) nunchukMap[i]=d[i]; }
 else if(control==2) { const int d[]={0,1,2,3,4,5,6,7,9,10,11,8}; for(int i=0;i<12;i++) classicMap[i]=d[i]; }
 else if(control==3) {
  const int r[]={0,1,2,3,11,12,13,8}; const int n[]={9,4,5};
  for(int i=0;i<8;i++) remoteMap[i]=r[i];
  for(int i=0;i<3;i++) nunchukMap[i]=n[i];
 }
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
 enum class Panel { Home, Settings, Controls, About, LoadGame, System };
 Panel panel=Panel::Home;
 while(aptMainLoop()) {
  hidScanInput(); const u32 down=hidKeysDown();
  if(down&KEY_START && panel==Panel::Home) break;
  if(panel==Panel::Home) {
   if(down&KEY_UP) selected=(selected+4)%5;
   if(down&KEY_DOWN) selected=(selected+1)%5;
   if(down&KEY_A) {
    if(selected==0) panel=Panel::System;
    else if(selected==1) panel=Panel::LoadGame;
    else if(selected==2) { panel=Panel::Controls; control=0; mappingRow=0; }
    else if(selected==3) { panel=Panel::Settings; settingsOpen=true; }
    else panel=Panel::About;
   }
   touchPosition touch{};
   if(down&KEY_TOUCH) { hidTouchRead(&touch); if(touch.px>=270 && touch.py<=42) { panel=Panel::Settings; settingsOpen=true; } }
   drawHome(top,bottom);
  } else if(panel==Panel::Settings) {
   touchPosition touch{};
   if(down&KEY_TOUCH) {
    hidTouchRead(&touch);
    if(touch.px>=270 && touch.py<=42) { panel=Panel::Home; settingsOpen=false; }
    else if(touch.py>=58 && touch.py<208) {
     int pick=(touch.py-58)/25;
     if(pick>=0&&pick<6) category=pick;
     if(category==0 && touch.py>=105) {
      int item=(touch.py-105)/16;
      if(item>=0&&item<6) { control=item; panel=Panel::Controls; mappingRow=0; }
     }
    }
   }
   if(down&KEY_B) { panel=Panel::Home; settingsOpen=false; }
   if(down&KEY_UP) category=(category+5)%6;
   if(down&KEY_DOWN) category=(category+1)%6;
   drawHome(top,bottom);
  } else if(panel==Panel::Controls) {
   if(down&KEY_B) panel=Panel::Home;
   if(down&KEY_UP) { control=(control+5)%6; mappingRow=0; }
   if(down&KEY_DOWN) { control=(control+1)%6; mappingRow=0; }
   if(down&KEY_L && mappingCount()>0) mappingRow=(mappingRow+mappingCount()-1)%mappingCount();
   if(down&KEY_R && mappingCount()>0) mappingRow=(mappingRow+1)%mappingCount();
   if(down&KEY_A && mappingCount()>0) cycleSelectedMapping();
   if(down&KEY_X && mappingCount()>0) resetMapping();
   touchPosition touch{};
   if(down&KEY_TOUCH && control<=3) {
    hidTouchRead(&touch);
    int row=(touch.py-30)/16;
    if(control==3) row=touch.py<160?(touch.py-48)/16:8+(touch.py-176)/16;
    if(row>=0&&row<mappingCount()) mappingRow=row;
   }
   drawControllers(top,bottom);
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