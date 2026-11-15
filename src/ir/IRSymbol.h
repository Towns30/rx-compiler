#pragma once

#include "../Ids.h"
#include "IRTypeTable.h"
#include "Type.h"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ir
{
using BlockId = std::size_t;

struct Value
{
  int value_id_;
  IRTypeId type_id_;
};

struct IntegerConst
{
  std::uint64_t value_; // 保存整数位模式
  IRTypeId type_id_;
};

using Operand = std::variant<Value, IntegerConst>;

inline IRTypeId getOperandTypeId(const Operand &operand)
{
  if (const auto *value = std::get_if<Value>(&operand))
  {
    return value->type_id_;
  }
  return std::get<IntegerConst>(operand).type_id_;
}

struct AllocaInst
{
  Value return_ptr_;
  IRTypeId type_id_;
};

struct LoadInst
{
  Value return_value_;
  IRTypeId type_id_;
  Value ptr_;
};

struct StoreInst
{
  IRTypeId type_id_;
  Operand operand_;
  Value ptr_;
};

enum class BinaryOp
{
  Add,
  Sub,
  Mul,
  SignedDiv,
  UnsignedDiv,
  SignedRem,
  UnsignedRem,
  BitAnd,
  BitOr,
  BitXor,
  ShiftLeft,
  LogicalShiftRight,   // 高位补零
  ArithmeticShiftRight // 高位补符号位
};

struct BinaryInst
{
  Value return_value_;
  BinaryOp op_;
  IRTypeId type_id_;
  Operand lhs_operand_;
  Operand rhs_operand_;
};

enum class CompareOp
{
  Equal,
  NotEqual,
  SignedLess,
  SignedLessEqual,
  SignedGreater,
  SignedGreaterEqual,
  UnsignedLess,
  UnsignedLessEqual,
  UnsignedGreater,
  UnsignedGreaterEqual
};

struct CompareInst
{
  Value return_value_;
  CompareOp op_;
  IRTypeId type_id_;
  Operand lhs_operand_;
  Operand rhs_operand_;
};

enum class CastOp
{
  Trunc,   // 截断，保留低位
  ZeroExt, // 零扩展
  SignExt  // 符号扩展
};

struct CastInst
{
  Value return_value_;
  CastOp op_;
  IRTypeId from_type_id_;
  Operand from_operand_;
  IRTypeId to_type_id_;
};

struct GetElementPtrInst
{
  Value return_ptr_;
  IRTypeId type_id_;
  Value ptr_;
  std::vector<Operand> indexs_;
};

struct CallInst
{
  std::optional<Value> return_value_ = std::nullopt;
  IRTypeId return_type_id_;
  FunctionId func_id_;
  std::vector<Operand> arg_operands_;
};

struct JumpInst
{
  BlockId target_block_id_;
};

struct BranchInst
{
  Operand cond_operand_;
  BlockId true_block_id_;
  BlockId false_block_id_;
};

struct ReturnInst
{
  std::optional<Operand> return_operand_ = std::nullopt;
};

struct PhiIncoming
{
  BlockId block_id_;
  Operand operand_;
};

struct PhiInst
{
  Value return_value_;
  std::vector<PhiIncoming> incomings_;
};

using Instruction = std::variant<AllocaInst, LoadInst, StoreInst, BinaryInst, CompareInst, CastInst, GetElementPtrInst,
                                 CallInst, JumpInst, BranchInst, ReturnInst, PhiInst>;

struct BasicBlock
{
  BlockId block_id_;
  std::vector<Instruction> instructions_;
};

struct Function
{
  FunctionId func_id_;
  IRTypeId return_type_id_;
  std::vector<Value> arg_values_;
  std::vector<BasicBlock> basic_blocks_;
};

struct Struct
{
  StructId struct_id_;
  std::vector<IRTypeId> types_;
};

class Module
{
public:
  Value newValue(IRTypeId type_id)
  {
    Value value{.value_id_ = next_value_id_, .type_id_ = type_id};
    next_value_id_++;
    return value;
  }

  BlockId newBlockId()
  {
    BlockId block_id = next_block_id_;
    next_block_id_++;
    return block_id;
  }

  IRTypeTable ir_type_table_;
  std::unordered_map<StructId, Struct> structs_;
  std::unordered_map<FunctionId, Function> functions_;

private:
  int next_value_id_ = 0;
  BlockId next_block_id_ = 0;
};

struct ExprResult
{
  bool terminated_ = false;
  std::optional<Operand> operand_ = std::nullopt;
};

} // namespace ir
