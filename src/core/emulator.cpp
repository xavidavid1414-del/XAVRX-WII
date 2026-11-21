#include "xavrx/core/emulator.hpp"

namespace xavrx {

bool Emulator::initialized() const noexcept {
    // Initialization becomes meaningful as hardware subsystems are implemented.
    return false;
}

} // namespace xavrx
