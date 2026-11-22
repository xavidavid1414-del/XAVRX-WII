#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
namespace xavrx::system {
class DspDevice final {
public:
 struct Task {std::uint32_t opcode{0U};std::uint64_t cycles{0U};};
 static constexpr std::size_t task_capacity=64U;
 [[nodiscard]] bool submit(Task task);
 void advance(std::uint64_t cycles) noexcept;
 void reset() noexcept;
 [[nodiscard]] bool busy() const noexcept{return !tasks_.empty();}
 [[nodiscard]] std::size_t queued_tasks() const noexcept{return tasks_.size();}
 [[nodiscard]] std::uint64_t completed_tasks() const noexcept{return completed_;}
 [[nodiscard]] std::uint32_t last_completed_opcode() const noexcept{return last_completed_opcode_;}
 [[nodiscard]] std::uint64_t active_cycles_remaining() const noexcept{return tasks_.empty()?0U:tasks_.front().cycles;}
private:
 std::deque<Task> tasks_{};
 std::uint64_t completed_{0U};
 std::uint32_t last_completed_opcode_{0U};
};
class GpuCommandProcessor final {
public:
 static constexpr std::size_t fifo_capacity=4096U;
 [[nodiscard]] bool submit(std::span<const std::uint32_t> words);
 [[nodiscard]] bool take(std::uint32_t& word) noexcept;
 void reset() noexcept{fifo_.clear();}
 [[nodiscard]] std::size_t queued_words() const noexcept{return fifo_.size();}
 [[nodiscard]] bool empty() const noexcept{return fifo_.empty();}
 [[nodiscard]] bool full() const noexcept{return fifo_.size()==fifo_capacity;}
private:
 std::deque<std::uint32_t> fifo_{};
};
}
