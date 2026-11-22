#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace xavrx::system {
enum class ClockDomain : std::uint8_t { Cpu, Gpu, Bus, Dsp, Starlet, Count };
class SystemClock final {
public:
 SystemClock() noexcept;
 explicit SystemClock(const std::array<std::uint64_t,static_cast<std::size_t>(ClockDomain::Count)>& hz) noexcept;
 [[nodiscard]] std::uint64_t frequency(ClockDomain domain) const noexcept;
 [[nodiscard]] bool valid() const noexcept{return valid_;}
 [[nodiscard]] std::uint64_t nanoseconds_for_cycles(ClockDomain domain,std::uint64_t cycles) const noexcept;
private:
 std::array<std::uint64_t,static_cast<std::size_t>(ClockDomain::Count)> hz_{};
 bool valid_{false};
};
class ClockDomainScheduler final {
public:
 static constexpr std::size_t domain_count=static_cast<std::size_t>(ClockDomain::Count);
 using Frequencies=std::array<std::uint64_t,domain_count>;
 [[nodiscard]] Frequencies advance(std::uint64_t source_cycles,std::uint64_t source_hz,const Frequencies& target_hz) noexcept;
 [[nodiscard]] std::uint64_t total_cycles(ClockDomain domain) const noexcept;
 void reset() noexcept{remainders_.fill(0U);totals_.fill(0U);}
private:
 std::array<std::uint64_t,domain_count> remainders_{};
 std::array<std::uint64_t,domain_count> totals_{};
};
class InterruptController final {
public:
 static constexpr std::size_t line_count=32;
 void raise(std::size_t line) noexcept;
 void clear(std::size_t line) noexcept;
 void set_enabled(std::size_t line,bool enabled) noexcept;
 [[nodiscard]] bool pending(std::size_t line) const noexcept;
 [[nodiscard]] bool enabled(std::size_t line) const noexcept;
 [[nodiscard]] std::uint32_t pending_mask() const noexcept{return pending_;}
 [[nodiscard]] std::uint32_t enabled_mask() const noexcept{return enabled_;}
 [[nodiscard]] std::uint32_t active_mask() const noexcept{return pending_&enabled_;}
 [[nodiscard]] int claim_highest_priority() noexcept;
 void reset() noexcept{pending_=0;enabled_=0;}
private:
 std::uint32_t pending_{0},enabled_{0};
};
class TimerBank final {
public:
 static constexpr std::size_t timer_count=4;
 explicit TimerBank(std::size_t interrupt_base=8) noexcept;
 [[nodiscard]] bool configure(std::size_t timer,std::uint64_t reload,bool periodic,bool enabled) noexcept;
 void enable(std::size_t timer,bool enabled) noexcept;
 void reset(std::size_t timer) noexcept;
 void advance(std::uint64_t cycles,InterruptController& interrupts) noexcept;
 [[nodiscard]] std::uint64_t remaining(std::size_t timer) const noexcept;
 [[nodiscard]] std::uint64_t expirations(std::size_t timer) const noexcept;
 [[nodiscard]] bool is_enabled(std::size_t timer) const noexcept;
 void reset_all() noexcept;
private:
 struct Timer {std::uint64_t reload{0},remaining{0},expirations{0};bool periodic{false},enabled{false};};
 std::array<Timer,timer_count> timers_{};
 std::size_t interrupt_base_{8};
};
enum class BootState : std::uint8_t { Off, BootRom, Firmware, Running, Halted, Faulted };
class BootController final {
public:
 [[nodiscard]] BootState state() const noexcept{return state_;}
 [[nodiscard]] bool transition(BootState next) noexcept;
 void reset() noexcept{state_=BootState::Off;}
private:
 BootState state_{BootState::Off};
};
class SystemHardware final {
public:
 SystemHardware() noexcept=default;
 [[nodiscard]] SystemClock& clock() noexcept{return clock_;}
 [[nodiscard]] const SystemClock& clock() const noexcept{return clock_;}
 [[nodiscard]] InterruptController& interrupts() noexcept{return interrupts_;}
 [[nodiscard]] const InterruptController& interrupts() const noexcept{return interrupts_;}
 [[nodiscard]] TimerBank& timers() noexcept{return timers_;}
 [[nodiscard]] const TimerBank& timers() const noexcept{return timers_;}
 [[nodiscard]] BootController& boot() noexcept{return boot_;}
 [[nodiscard]] const BootController& boot() const noexcept{return boot_;}
 void advance(std::uint64_t cpu_cycles) noexcept;
 void reset() noexcept;
private:
 SystemClock clock_{};
 InterruptController interrupts_{};
 TimerBank timers_{};
 BootController boot_{};
};
}
