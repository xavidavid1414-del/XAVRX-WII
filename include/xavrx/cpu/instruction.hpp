#pragma once
#include <cstdint>
namespace xavrx::cpu {
enum class InstructionId : std::uint16_t {
 Unknown,Addi,Addis,Ori,Oris,Xori,Xoris,AndiDot,Branch,BranchConditional,LoadWord,StoreWord,
 Add,Subf,Or,Xor,And,Compare,MoveFromCr,MoveToCrFields,MoveFromSpr,MoveToSpr,
 BranchLinkRegister,BranchCountRegister,InstructionSync,ReturnFromInterrupt,SystemCall,
 FloatAdd,FloatSubtract,FloatMultiply,FloatDivide
};
enum class InstructionForm : std::uint8_t {Unknown,D,I,B,X,XL,XO,A};
struct DecodedInstruction final {
 InstructionId id{InstructionId::Unknown}; InstructionForm form{InstructionForm::Unknown};
 std::uint8_t primary_opcode{}; std::uint16_t extended_opcode{}; bool has_extended_opcode{};
 [[nodiscard]] constexpr bool valid() const noexcept{return id!=InstructionId::Unknown;}
};
}