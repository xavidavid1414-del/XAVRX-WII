#include "xavrx/cpu/decoder.hpp"
namespace xavrx::cpu {
namespace {
constexpr DecodedInstruction simple(InstructionId id,InstructionForm form,std::uint8_t op) noexcept{return{id,form,op,0U,false};}
constexpr DecodedInstruction ext(InstructionId id,InstructionForm form,std::uint8_t op,std::uint16_t xo) noexcept{return{id,form,op,xo,true};}
}
DecodedInstruction InstructionDecoder::decode(std::uint32_t word) noexcept{
 const auto op=static_cast<std::uint8_t>(word>>26U);
 switch(op){
 case 14:return simple(InstructionId::Addi,InstructionForm::D,op);
 case 15:return simple(InstructionId::Addis,InstructionForm::D,op);
 case 16:return simple(InstructionId::BranchConditional,InstructionForm::B,op);
 case 17:return simple(InstructionId::SystemCall,InstructionForm::Unknown,op);
 case 18:return simple(InstructionId::Branch,InstructionForm::I,op);
 case 24:return simple(InstructionId::Ori,InstructionForm::D,op);
 case 25:return simple(InstructionId::Oris,InstructionForm::D,op);
 case 26:return simple(InstructionId::Xori,InstructionForm::D,op);
 case 27:return simple(InstructionId::Xoris,InstructionForm::D,op);
 case 28:return simple(InstructionId::AndiDot,InstructionForm::D,op);
 case 32:return simple(InstructionId::LoadWord,InstructionForm::D,op);
 case 36:return simple(InstructionId::StoreWord,InstructionForm::D,op);
 case 19:{const auto xo=static_cast<std::uint16_t>((word>>1U)&0x3FFU);switch(xo){
 case 16:return ext(InstructionId::BranchLinkRegister,InstructionForm::XL,op,xo);
 case 50:return ext(InstructionId::ReturnFromInterrupt,InstructionForm::XL,op,xo);
 case 150:return ext(InstructionId::InstructionSync,InstructionForm::XL,op,xo);
 case 528:return ext(InstructionId::BranchCountRegister,InstructionForm::XL,op,xo);
 default:return{};}}
 case 31:{const auto xo=static_cast<std::uint16_t>((word>>1U)&0x3FFU);switch(xo){
 case 0:return ext(InstructionId::Compare,InstructionForm::X,op,xo);
 case 19:return ext(InstructionId::MoveFromCr,InstructionForm::X,op,xo);
 case 28:return ext(InstructionId::And,InstructionForm::X,op,xo);
 case 40:return ext(InstructionId::Subf,InstructionForm::XO,op,xo);
 case 144:return ext(InstructionId::MoveToCrFields,InstructionForm::X,op,xo);
 case 266:return ext(InstructionId::Add,InstructionForm::XO,op,xo);
 case 316:return ext(InstructionId::Xor,InstructionForm::X,op,xo);
 case 339:return ext(InstructionId::MoveFromSpr,InstructionForm::X,op,xo);
 case 444:return ext(InstructionId::Or,InstructionForm::X,op,xo);
 case 467:return ext(InstructionId::MoveToSpr,InstructionForm::X,op,xo);
 default:return{};}}
 case 63:{const auto xo=static_cast<std::uint16_t>((word>>1U)&0x3FFU);switch(xo){
 case 18:return ext(InstructionId::FloatDivide,InstructionForm::A,op,xo);
 case 20:return ext(InstructionId::FloatSubtract,InstructionForm::A,op,xo);
 case 21:return ext(InstructionId::FloatAdd,InstructionForm::A,op,xo);
 case 25:return ext(InstructionId::FloatMultiply,InstructionForm::A,op,xo);
 default:return{};}}
 default:return{};
 }
}
}