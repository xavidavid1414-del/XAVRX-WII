#include "xavrx/system/hardware.hpp"
#include <limits>
namespace xavrx::system {
namespace {
constexpr std::size_t index(ClockDomain d) noexcept{return static_cast<std::size_t>(d);}
constexpr std::uint64_t nanoseconds_per_second=1000000000ULL;
// Computes floor(a * b / d) without overflowing when a < d.
constexpr std::uint64_t multiply_divide_fraction(std::uint64_t a,std::uint64_t b,std::uint64_t d) noexcept {
 std::uint64_t quotient=0U,remainder=0U;
 for(int bit_index=63;bit_index>=0;--bit_index){
  quotient*=2U;
  if(remainder>=d-remainder){remainder-=d-remainder;++quotient;}
  else remainder+=remainder;
  if(((b>>static_cast<unsigned>(bit_index))&1U)!=0U){
   if(remainder>=d-a){remainder-=d-a;++quotient;}
   else remainder+=a;
  }
 }
 return quotient;
}
constexpr std::uint64_t bit(std::size_t line) noexcept{return std::uint32_t{1}<<line;}
}
SystemClock::SystemClock() noexcept:hz_{729000000ULL,243000000ULL,162000000ULL,121500000ULL,243000000ULL},valid_(true){}
SystemClock::SystemClock(const std::array<std::uint64_t,index(ClockDomain::Count)>& hz) noexcept:hz_(hz){
 valid_=true;for(const auto value:hz_)if(value==0U)valid_=false;
}
std::uint64_t SystemClock::frequency(ClockDomain domain)const noexcept{
 const auto i=index(domain);return i<hz_.size()?hz_[i]:0U;
}
std::uint64_t SystemClock::nanoseconds_for_cycles(ClockDomain domain,std::uint64_t cycles)const noexcept{
 const auto hz=frequency(domain);if(hz==0U)return 0U;
 const auto whole=cycles/hz,part=cycles%hz;
 if(whole>std::numeric_limits<std::uint64_t>::max()/nanoseconds_per_second)return std::numeric_limits<std::uint64_t>::max();
 const auto base=whole*nanoseconds_per_second;
 const auto fraction=multiply_divide_fraction(part,nanoseconds_per_second,hz);
 if(base>std::numeric_limits<std::uint64_t>::max()-fraction)return std::numeric_limits<std::uint64_t>::max();
 return base+fraction;
}
ClockDomainScheduler::Frequencies ClockDomainScheduler::advance(std::uint64_t source_cycles,std::uint64_t source_hz,const Frequencies& target_hz)noexcept{
 Frequencies produced{};
 if(source_hz==0U||source_cycles==0U)return produced;
 const auto whole=source_cycles/source_hz;
 const auto part=source_cycles%source_hz;
 for(std::size_t i=0;i<domain_count;++i){
  const auto target=target_hz[i];if(target==0U)continue;
  std::uint64_t base=0U;
  if(whole!=0U&&target>std::numeric_limits<std::uint64_t>::max()/whole)base=std::numeric_limits<std::uint64_t>::max();
  else base=whole*target;
  std::uint64_t fractional=0U;
  if(part!=0U){
   if(target>(std::numeric_limits<std::uint64_t>::max()-remainders_[i])/part){
    fractional=std::numeric_limits<std::uint64_t>::max();remainders_[i]=0U;
   }else{
    const auto numerator=part*target+remainders_[i];
    fractional=numerator/source_hz;remainders_[i]=numerator%source_hz;
   }
  }
  const auto delta=base>std::numeric_limits<std::uint64_t>::max()-fractional?std::numeric_limits<std::uint64_t>::max():base+fractional;
  produced[i]=delta;
  totals_[i]=totals_[i]>std::numeric_limits<std::uint64_t>::max()-delta?std::numeric_limits<std::uint64_t>::max():totals_[i]+delta;
 }
 return produced;
}
std::uint64_t ClockDomainScheduler::total_cycles(ClockDomain domain)const noexcept{
 const auto i=index(domain);return i<totals_.size()?totals_[i]:0U;
}
void InterruptController::raise(std::size_t line)noexcept{if(line<line_count)pending_|=static_cast<std::uint32_t>(bit(line));}
void InterruptController::clear(std::size_t line)noexcept{if(line<line_count)pending_&=~static_cast<std::uint32_t>(bit(line));}
void InterruptController::set_enabled(std::size_t line,bool value)noexcept{
 if(line>=line_count)return;
  const auto mask=static_cast<std::uint32_t>(bit(line));if(value)enabled_|=mask;else enabled_&=~mask;
}
bool InterruptController::pending(std::size_t line)const noexcept{return line<line_count&&(pending_&bit(line))!=0U;}
bool InterruptController::enabled(std::size_t line)const noexcept{return line<line_count&&(enabled_&bit(line))!=0U;}
int InterruptController::claim_highest_priority()noexcept{
 const auto active=active_mask();if(active==0U)return -1;
 for(std::size_t line=0;line<line_count;++line)if((active&bit(line))!=0U){clear(line);return static_cast<int>(line);}
 return -1;
}
TimerBank::TimerBank(std::size_t base)noexcept:interrupt_base_(base){}
bool TimerBank::configure(std::size_t timer,std::uint64_t reload,bool periodic,bool enabled_value)noexcept{
 if(timer>=timer_count||reload==0U||interrupt_base_>=InterruptController::line_count||timer>=InterruptController::line_count-interrupt_base_)return false;
 timers_[timer]={reload,reload,0U,periodic,enabled_value};return true;
}
void TimerBank::enable(std::size_t timer,bool value)noexcept{
 if(timer>=timer_count)return;
  auto& t=timers_[timer];if(value&&t.reload==0U)return;if(value&&!t.enabled&&t.remaining==0U)t.remaining=t.reload;t.enabled=value;
}
void TimerBank::reset(std::size_t timer)noexcept{if(timer<timer_count)timers_[timer]=Timer{};}
void TimerBank::advance(std::uint64_t cycles,InterruptController& interrupts)noexcept{
 if(cycles==0U)return;
 for(std::size_t i=0;i<timer_count;++i){
  auto& t=timers_[i];if(!t.enabled)continue;
  if(cycles<t.remaining){t.remaining-=cycles;continue;}
  const auto elapsed=cycles-t.remaining;interrupts.raise(interrupt_base_+i);
  if(!t.periodic){if(t.expirations!=std::numeric_limits<std::uint64_t>::max())++t.expirations;t.remaining=0U;t.enabled=false;continue;}
  const auto extra=elapsed/t.reload;const auto count=extra+1U;
  if(t.expirations>std::numeric_limits<std::uint64_t>::max()-count)t.expirations=std::numeric_limits<std::uint64_t>::max();else t.expirations+=count;
  const auto remainder=elapsed%t.reload;t.remaining=remainder==0U?t.reload:t.reload-remainder;
 }
}
std::uint64_t TimerBank::remaining(std::size_t timer)const noexcept{return timer<timer_count?timers_[timer].remaining:0U;}
std::uint64_t TimerBank::expirations(std::size_t timer)const noexcept{return timer<timer_count?timers_[timer].expirations:0U;}
bool TimerBank::is_enabled(std::size_t timer)const noexcept{return timer<timer_count&&timers_[timer].enabled;}
void TimerBank::reset_all()noexcept{for(auto& t:timers_)t=Timer{};}
bool BootController::transition(BootState next)noexcept{
 bool allowed=false;
 switch(state_){
 case BootState::Off:allowed=next==BootState::BootRom;break;
 case BootState::BootRom:allowed=next==BootState::Firmware||next==BootState::Faulted;break;
 case BootState::Firmware:allowed=next==BootState::Running||next==BootState::Faulted;break;
 case BootState::Running:allowed=next==BootState::Halted||next==BootState::Faulted;break;
 case BootState::Halted:allowed=next==BootState::Running||next==BootState::Faulted;break;
 case BootState::Faulted:allowed=next==BootState::Off;break;
 }
 if(allowed)state_=next;
  return allowed;
}
void SystemHardware::advance(std::uint64_t cycles)noexcept{timers_.advance(cycles,interrupts_);}
void SystemHardware::reset()noexcept{interrupts_.reset();timers_.reset_all();boot_.reset();}
}
