#pragma once

#include "Type.h"
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace ir
{
using BlockId = std::size_t;
using FunctionId = std::size_t;

class Value
{
public:
  int value_id_;
  IRTypeId type_id_;
};

class IntegerConst
{
public:
  std::uint64_t value_; // 保存整数位模式
  IRTypeId type_id_;
};

using Operand = std::variant<Value, IntegerConst>;

class AllocaInst
{
public:
  Value return_ptr_;
  IRTypeId type_id_;
};

class LoadInst
{
public:
  Value return_value_;
  IRTypeId type_id_;
  Value ptr_;
};

class StoreInst
{
public:
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

class BinaryInst
{
public:
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

class CompareInst
{
public:
  Value return_value_;
  CompareOp op_;
  IRTypeId type_id_;
  Operand lhs_operand_;
  Operand rhs_operand_;
};

enum class CastOp
{
  Trunc,    // 截断，保留低位
  ZeroExt,  // 零扩展
  SignExt   // 符号扩展
};

class CastInst
{
public:
  Value return_value_;
  CastOp op_;
  IRTypeId from_type_id_;
  Operand from_operand_;
  IRTypeId to_type_id_;
};

class GetElementPtrInst
{
public:
  Value return_ptr_;
  IRTypeId type_id_;
  Value ptr_;
  std::vector<Operand> indexs_;
};

class CallInst
{
public:
  std::optional<Value> return_value_ = std::nullopt;
  IRTypeId return_type_id_;
  FunctionId func_id_;
  std::vector<Operand> arg_operands_;
};

class JumpInst
{
public:
  BlockId target_block_id_;
};

class BranchInst
{
public:
  Operand cond_operand_;
  BlockId true_block_id_;
  BlockId false_block_id_;
};

class ReturnInst
{
public:
  std::optional<Operand> return_operand_ = std::nullopt;
};

using Instruction = std::variant<
    AllocaInst, LoadInst, StoreInst, BinaryInst, CompareInst, CastInst,
    GetElementPtrInst, CallInst, JumpInst, BranchInst, ReturnInst>;

class BasicBlock
{
public:
  std::vector<Instruction> instructions_;
};

class Function
{
public:
  FunctionId func_id_;
  IRTypeId return_type_id_;
  std::vector<Value> arg_values_;
  std::vector<BasicBlock> basic_blocks_;
};

class Module
{
public:
  std::vector<Function> functions_;
};

} // namespace ir
