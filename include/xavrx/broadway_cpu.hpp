#pragma once

#include "xavrx/memory.hpp"

#include <array>
#include <cstdint>

namespace xavrx {

class BroadwayCpu {
public:
    struct State {
        std::array<std::uint32_t, 32> gpr{};
        std::uint32_t pc{0};
        std::uint32_t lr{0};
        std::uint32_t ctr{0};
        std::uint32_t cr{0};
        std::uint32_t xer{0};
    };

    explicit BroadwayCpu(Memory& memory, std::uint32_t entry = 0) noexcept;

    // Executes one supported PowerPC instruction. Returns false for an
    // unsupported opcode; the PC and registers remain unchanged.
    [[nodiscard]] bool step();
    [[nodiscard]] const State& state() const noexcept;
    void set_state(const State& state) noexcept;

private:
    Memory& memory_;
    State state_{};
};

} // namespace xavrx
