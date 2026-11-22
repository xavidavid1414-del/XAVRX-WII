#include "xavrx/system/dsp_gpu.hpp"
#include <limits>
namespace xavrx::system {
bool DspDevice::submit(Task task){
 if(task.cycles==0U||tasks_.size()>=task_capacity)return false;
 tasks_.push_back(task);return true;
}
void DspDevice::advance(std::uint64_t cycles) noexcept {
 while(cycles!=0U&&!tasks_.empty()){
  auto& task=tasks_.front();
  if(cycles<task.cycles){task.cycles-=cycles;return;}
  cycles-=task.cycles;last_completed_opcode_=task.opcode;
  if(completed_!=std::numeric_limits<std::uint64_t>::max())++completed_;
  tasks_.pop_front();
 }
}
void DspDevice::reset() noexcept {tasks_.clear();completed_=0U;last_completed_opcode_=0U;}
bool GpuCommandProcessor::submit(std::span<const std::uint32_t> words){
 if(words.size()>fifo_capacity-fifo_.size())return false;
 for(const auto word:words)fifo_.push_back(word);
 return true;
}
bool GpuCommandProcessor::take(std::uint32_t& word) noexcept {
 if(fifo_.empty())return false;
 word=fifo_.front();fifo_.pop_front();return true;
}
}
