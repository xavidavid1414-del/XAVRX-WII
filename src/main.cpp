#include "xavrx/core/emulator.hpp"
#include <iostream>
#include <string_view>
int main(int argc,char** argv){
 if(argc>1&&std::string_view{argv[1]}=="--version"){
  std::cout<<"XAVRX-WII "<<xavrx::Emulator::version()<<'\n';return 0;
 }
 xavrx::Emulator emulator;
 if(!emulator.initialize()){
  std::cerr<<"XAVRX-WII hardware initialization failed\n";return 1;
 }
 std::cout<<"XAVRX-WII "<<xavrx::Emulator::version()<<'\n'
          <<"Wii hardware foundation initialized. CPU execution, firmware boot, graphics, and audio integration are still in development.\n";
 return 0;
}
