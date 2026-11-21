#pragma once

#include "xavrx/memory.hpp"

#include <array>
#include <cstdint>

namespace xavrx {

class BroadwayCpu {
public:
    enum class Exception : std::uint8_t {
        None,
        Alignment,
        InstructionStorage,
        DataStorage,
        IllegalInstruction
    };

    struct State {
        std::array<std::uint32_t, 32> gpr{};
        // FPRs are stored as IEEE-754 binary64 bit patterns.
        std::array<std::uint64_t, 32> fpr{};
        std::uint32_t fpscr{0};
        Exception exception{Exception::None};
        std::uint32_t pc{0};
        std::uint32_t lr{0};
        std::uint32_t ctr{0};
        std::uint32_t cr{0};
        std::uint32_t xer{0};
        std::uint32_t reservation_address{0};
        bool reservation_valid{false};
    };

    explicit BroadwayCpu(Memory& memory, std::uint32_t entry = 0) noexcept;

    // Executes one supported PowerPC instruction. Returns false for an
    // unsupported opcode; the PC and registers remain unchanged.
    [[nodiscard]] bool step();
    [[nodiscard]] const State& state() const noexcept;
    void set_state(const State& state) noexcept;

private:
    [[nodiscard]] bool execute_one();

    Memory& memory_;
    State state_{};
};

} // namespace xavrx
