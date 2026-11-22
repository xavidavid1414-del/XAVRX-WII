#pragma once
#include "xavrx/memory/memory.hpp"
#include "xavrx/system/dsp_gpu.hpp"
#include <cstdint>
namespace xavrx::system {
// Broadway DSP mailbox interface. Only documented mailbox registers are exposed.
class DspMailboxInterface final : public memory::MemoryMappedDevice {
public:
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 [[nodiscard]] bool take_mail_to_dsp(std::uint32_t& value) noexcept;
 [[nodiscard]] bool post_mail_from_dsp(std::uint32_t value) noexcept;
 void reset() noexcept;
private:
 std::uint16_t in_high_{0},in_low_{0},out_high_{0},out_low_{0};
 bool input_pending_{false},output_pending_{false};
};
// Broadway processor-interface interrupt cause/mask registers.
class ProcessorInterruptInterface final : public memory::MemoryMappedDevice {
public:
 [[nodiscard]] bool read(std::uint32_t offset,std::uint8_t width,std::uint64_t& value) noexcept override;
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
 void raise(std::uint8_t source) noexcept;
 void reset() noexcept { cause_=0; mask_=0; }
 [[nodiscard]] std::uint32_t cause() const noexcept{return cause_;}
 [[nodiscard]] std::uint32_t mask() const noexcept{return mask_;}
 [[nodiscard]] std::uint32_t active() const noexcept{return cause_&mask_;}
private:
 std::uint32_t cause_{0},mask_{0};
};
// Write-gather pipe: each 32-bit store contributes one ordered GX FIFO word.
class GxWriteGatherPort final : public memory::MemoryMappedDevice {
public:
 explicit GxWriteGatherPort(GpuCommandProcessor& processor) noexcept:processor_(processor){}
 [[nodiscard]] bool read(std::uint32_t,std::uint8_t,std::uint64_t&) noexcept override{return false;}
 [[nodiscard]] bool write(std::uint32_t offset,std::uint8_t width,std::uint64_t value) noexcept override;
private:
 GpuCommandProcessor& processor_;
};
}
