#include "xavrx/system/wii_system.hpp"
namespace xavrx::system {
bool WiiSystem::initialize()noexcept{
 if(initialized_)return true;
 if(!hollywood_.map_into(memory_))return false;
 initialized_=true;return true;
}
void WiiSystem::reset()noexcept{
 memory_.reset();hardware_.reset();hollywood_.reset();
}
}
