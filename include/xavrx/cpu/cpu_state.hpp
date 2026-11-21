#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace xavrx::cpu {
class CpuState final {
public:
 static constexpr std::size_t register_count=32, condition_field_count=8;
 [[nodiscard]] std::uint32_t gpr(std::size_t) const noexcept; void set_gpr(std::size_t,std::uint32_t) noexcept;
 [[nodiscard]] std::uint64_t fpr(std::size_t) const noexcept; void set_fpr(std::size_t,std::uint64_t) noexcept;
 [[nodiscard]] std::uint32_t condition_register() const noexcept; void set_condition_register(std::uint32_t) noexcept;
 [[nodiscard]] std::uint8_t condition_field(std::size_t) const noexcept; void set_condition_field(std::size_t,std::uint8_t) noexcept;
 [[nodiscard]] std::uint32_t pc() const noexcept{return pc_;} void set_pc(std::uint32_t v) noexcept{pc_=v;}
 [[nodiscard]] std::uint32_t lr() const noexcept{return lr_;} void set_lr(std::uint32_t v) noexcept{lr_=v;}
 [[nodiscard]] std::uint32_t ctr() const noexcept{return ctr_;} void set_ctr(std::uint32_t v) noexcept{ctr_=v;}
 [[nodiscard]] std::uint32_t xer() const noexcept{return xer_;} void set_xer(std::uint32_t v) noexcept{xer_=v;}
 [[nodiscard]] std::uint32_t msr() const noexcept{return msr_;} void set_msr(std::uint32_t v) noexcept{msr_=v;}
 [[nodiscard]] std::uint32_t srr0() const noexcept{return srr0_;} void set_srr0(std::uint32_t v) noexcept{srr0_=v;}
 [[nodiscard]] std::uint32_t srr1() const noexcept{return srr1_;} void set_srr1(std::uint32_t v) noexcept{srr1_=v;}
 [[nodiscard]] std::uint32_t fpscr() const noexcept{return fpscr_;} void set_fpscr(std::uint32_t v) noexcept{fpscr_=v;}
 void reset() noexcept;
private:
 std::array<std::uint32_t,32> gpr_{}; std::array<std::uint64_t,32> fpr_{};
 std::uint32_t pc_{},lr_{},ctr_{},xer_{},msr_{},srr0_{},srr1_{},fpscr_{},cr_{};
};
}