#include "xavrx/system/wii_system.hpp"
namespace xavrx::system {
WiiSystem::WiiSystem()
 :broadway_io_bus_(0x80000000U,1U),starlet_io_bus_(0x80000000U,1U){}
bool WiiSystem::initialize()noexcept{
 if(initialized_)return true;
 if(!broadway_io_bus_.valid()||!starlet_io_bus_.valid())return false;
 if(!hollywood_.map_into(broadway_io_bus_,starlet_io_bus_))return false;
 initialized_=true;return true;
}
void WiiSystem::reset()noexcept{
 memory_.reset();hardware_.reset();hollywood_.reset();
}
}
