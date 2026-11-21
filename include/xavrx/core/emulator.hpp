#pragma once

#include <string_view>

namespace xavrx {

class Emulator {
public:
    [[nodiscard]] static constexpr std::string_view version() noexcept {
        return "0.1.0";
    }

    [[nodiscard]] bool initialized() const noexcept;
};

} // namespace xavrx
