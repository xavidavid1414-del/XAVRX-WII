#include "xavrx/core/emulator.hpp"
#include <memory>
namespace xavrx {
bool Emulator::initialize()noexcept{
 if(system_)return system_->initialized();
 try{
  auto candidate=std::make_unique<system::WiiSystem>();
  if(!candidate->initialize())return false;
  system_=std::move(candidate);return true;
 }catch(...){return false;}
}
bool Emulator::initialized()const noexcept{return system_&&system_->initialized();}
void Emulator::reset()noexcept{if(system_)system_->reset();}
}
