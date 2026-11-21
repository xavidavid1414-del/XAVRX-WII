#include "xavrx/cpu/cpu_state.hpp"
namespace xavrx::cpu {
std::uint32_t CpuState::gpr(std::size_t i) const noexcept{return i<register_count?gpr_[i]:0U;}
void CpuState::set_gpr(std::size_t i,std::uint32_t v) noexcept{if(i<register_count)gpr_[i]=v;}
std::uint64_t CpuState::fpr(std::size_t i) const noexcept{return i<register_count?fpr_[i]:0ULL;}
void CpuState::set_fpr(std::size_t i,std::uint64_t v) noexcept{if(i<register_count)fpr_[i]=v;}
std::uint32_t CpuState::condition_register() const noexcept{return cr_;}
void CpuState::set_condition_register(std::uint32_t v) noexcept{cr_=v;}
std::uint8_t CpuState::condition_field(std::size_t i) const noexcept{
 if(i>=condition_field_count)return 0U;
 const auto shift=static_cast<unsigned>((7U-i)*4U);
 return static_cast<std::uint8_t>((cr_>>shift)&0xFU);
}
void CpuState::set_condition_field(std::size_t i,std::uint8_t v) noexcept{
 if(i>=condition_field_count)return;
 const auto shift=static_cast<unsigned>((7U-i)*4U);
 const auto mask=static_cast<std::uint32_t>(0xFU<<shift);
 cr_=(cr_&~mask)|(static_cast<std::uint32_t>(v&0xFU)<<shift);
}
void CpuState::reset() noexcept{
 gpr_.fill(0U);fpr_.fill(0ULL);pc_=lr_=ctr_=xer_=msr_=srr0_=srr1_=fpscr_=cr_=0U;
}
}