#include "IRPrinter.h"
#include <stdexcept>

namespace ir
{
IRPrinter::IRPrinter(IRContext &ir_context, std::ostream &output)
    : ir_context_(ir_context), output_(output)
{
}

// 函数定义和调用统一使用编号作为符号名。
void IRPrinter::printFunctionName(FunctionId func_id)
{
  output_ << "@f" << func_id;
}

void IRPrinter::printIR(const Module &module)
{
  output_ << "target triple = \"riscv32-unknown-elf\"\n\n";
  // 打印所有结构体定义
  for (IRTypeId type_id = 0; type_id < ir_context_.getTypeCount(); type_id++)
  {
    IRType type = ir_context_.getType(type_id);
    const auto *struct_type = std::get_if<StructIRType>(&type);
    if (!struct_type)
    {
      continue;
    }
    output_ << "%s" << type_id << " = type { ";
    for (std::size_t i = 0; i < struct_type->fields_.size(); i++)
    {
      if (i != 0)
      {
        output_ << ", ";
      }
      printType(struct_type->fields_[i]);
    }
    output_ << " }\n";
  }
  output_ << '\n';
  for (const Function &function : module.functions_)
  {
    printFunction(function);
    output_ << '\n';
  }
}

void IRPrinter::printFunction(const Function &function)
{
  const bool is_declaration = function.basic_blocks_.empty();
  if (is_declaration)
  {
    output_ << "declare ";
  }
  else
  {
    output_ << "define ";
  }
  printType(function.return_type_id_);
  output_ << ' ';
  printFunctionName(function.func_id_);
  output_ << '(';
  for (std::size_t i = 0; i < function.arg_values_.size(); i++)
  {
    if (i != 0)
    {
      output_ << ", ";
    }
    const Value &arg = function.arg_values_[i];
    printType(arg.type_id_);
    if (!is_declaration)
    {
      output_ << ' ';
      printOperand(arg);
    }
  }
  output_ << ')';
  if (is_declaration)
  {
    output_ << '\n';
    return;
  }
  output_ << " {\n";
  for (BlockId block_id = 0; block_id < function.basic_blocks_.size(); block_id++)
  {
    output_ << "bb" << block_id << ":\n";
    for (const Instruction &instruction : function.basic_blocks_[block_id].instructions_)
    {
      output_ << "  ";
      printInstruction(instruction);
    }
  }
  output_ << "}\n";
}

void IRPrinter::printType(IRTypeId type_id)
{
  IRType type = ir_context_.getType(type_id);
  if (std::get_if<VoidIRType>(&type))
  {
    output_ << "void";
  }
  else if (const auto *integer_type = std::get_if<IntegerIRType>(&type))
  {
    output_ << 'i' << integer_type->bit_width_;
  }
  else if (std::get_if<PointerIRType>(&type))
  {
    output_ << "ptr";
  }
  else if (const auto *array_type = std::get_if<ArrayIRType>(&type))
  {
    output_ << '[' << array_type->length_ << " x ";
    printType(array_type->element_type_id_);
    output_ << ']';
  }
  else if (std::get_if<StructIRType>(&type))
  {
    // 使用类型编号区分同名结构体，定义在模块打印阶段输出
    output_ << "%s" << type_id;
  }
}

void IRPrinter::printOperand(const Operand &operand)
{
  if (const auto *value = std::get_if<Value>(&operand))
  {
    output_ << "%v" << value->value_id_;
    return;
  }
  const auto &constant = std::get<IntegerConst>(operand);
  IRType type = ir_context_.getType(constant.type_id_);
  const auto *integer_type = std::get_if<IntegerIRType>(&type);
  if (!integer_type || integer_type->bit_width_ == 0 ||
      integer_type->bit_width_ > 64)
  {
    throw std::logic_error("Integer constant requires a width from 1 to 64");
  }
  std::uint64_t bits = constant.value_;
  if (integer_type->bit_width_ < 64)
  {
    bits &= (std::uint64_t{1} << integer_type->bit_width_) - 1;
  }
  output_ << bits;
}

static const char *getBinaryName(BinaryOp op)
{
  switch (op)
  {
  case BinaryOp::Add: return "add";
  case BinaryOp::Sub: return "sub";
  case BinaryOp::Mul: return "mul";
  case BinaryOp::SignedDiv: return "sdiv";
  case BinaryOp::UnsignedDiv: return "udiv";
  case BinaryOp::SignedRem: return "srem";
  case BinaryOp::UnsignedRem: return "urem";
  case BinaryOp::BitAnd: return "and";
  case BinaryOp::BitOr: return "or";
  case BinaryOp::BitXor: return "xor";
  case BinaryOp::ShiftLeft: return "shl";
  case BinaryOp::LogicalShiftRight: return "lshr";
  case BinaryOp::ArithmeticShiftRight: return "ashr";
  }
}

static const char *getCompareName(CompareOp op)
{
  switch (op)
  {
  case CompareOp::Equal: return "eq";
  case CompareOp::NotEqual: return "ne";
  case CompareOp::SignedLess: return "slt";
  case CompareOp::SignedLessEqual: return "sle";
  case CompareOp::SignedGreater: return "sgt";
  case CompareOp::SignedGreaterEqual: return "sge";
  case CompareOp::UnsignedLess: return "ult";
  case CompareOp::UnsignedLessEqual: return "ule";
  case CompareOp::UnsignedGreater: return "ugt";
  case CompareOp::UnsignedGreaterEqual: return "uge";
  }
}

static const char *getCastName(CastOp op)
{
  switch (op)
  {
  case CastOp::Trunc: return "trunc";
  case CastOp::ZeroExt: return "zext";
  case CastOp::SignExt: return "sext";
  }
}

static IRTypeId getOperandTypeId(const Operand &operand)
{
  if (const auto *value = std::get_if<Value>(&operand))
  {
    return value->type_id_;
  }
  return std::get<IntegerConst>(operand).type_id_;
}

void IRPrinter::printInstruction(const Instruction &instruction)
{
  if (const auto *inst = std::get_if<AllocaInst>(&instruction))
  {
    output_ << "%v" << inst->return_ptr_.value_id_ << " = alloca ";
    printType(inst->type_id_);
  }
  else if (const auto *inst = std::get_if<LoadInst>(&instruction))
  {
    output_ << "%v" << inst->return_value_.value_id_ << " = load ";
    printType(inst->type_id_);
    output_ << ", ptr ";
    printOperand(inst->ptr_);
  }
  else if (const auto *inst = std::get_if<StoreInst>(&instruction))
  {
    output_ << "store ";
    printType(inst->type_id_);
    output_ << ' ';
    printOperand(inst->operand_);
    output_ << ", ptr ";
    printOperand(inst->ptr_);
  }
  else if (const auto *inst = std::get_if<BinaryInst>(&instruction))
  {
    output_ << "%v" << inst->return_value_.value_id_ << " = "
            << getBinaryName(inst->op_) << ' ';
    printType(inst->type_id_);
    output_ << ' ';
    printOperand(inst->lhs_operand_);
    output_ << ", ";
    printOperand(inst->rhs_operand_);
  }
  else if (const auto *inst = std::get_if<CompareInst>(&instruction))
  {
    output_ << "%v" << inst->return_value_.value_id_ << " = icmp "
            << getCompareName(inst->op_) << ' ';
    printType(inst->type_id_);
    output_ << ' ';
    printOperand(inst->lhs_operand_);
    output_ << ", ";
    printOperand(inst->rhs_operand_);
  }
  else if (const auto *inst = std::get_if<CastInst>(&instruction))
  {
    output_ << "%v" << inst->return_value_.value_id_ << " = "
            << getCastName(inst->op_) << ' ';
    printType(inst->from_type_id_);
    output_ << ' ';
    printOperand(inst->from_operand_);
    output_ << " to ";
    printType(inst->to_type_id_);
  }
  else if (const auto *inst = std::get_if<GetElementPtrInst>(&instruction))
  {
    output_ << "%v" << inst->return_ptr_.value_id_ << " = getelementptr ";
    printType(inst->type_id_);
    output_ << ", ptr ";
    printOperand(inst->ptr_);
    for (const Operand &index : inst->indexs_)
    {
      output_ << ", ";
      printType(getOperandTypeId(index));
      output_ << ' ';
      printOperand(index);
    }
  }
  else if (const auto *inst = std::get_if<CallInst>(&instruction))
  {
    if (inst->return_value_.has_value())
    {
      printOperand(inst->return_value_.value());
      output_ << " = ";
    }
    output_ << "call ";
    printType(inst->return_type_id_);
    output_ << ' ';
    printFunctionName(inst->func_id_);
    output_ << '(';
    for (std::size_t i = 0; i < inst->arg_operands_.size(); i++)
    {
      if (i != 0)
      {
        output_ << ", ";
      }
      const Operand &arg = inst->arg_operands_[i];
      printType(getOperandTypeId(arg));
      output_ << ' ';
      printOperand(arg);
    }
    output_ << ')';
  }
  else if (const auto *inst = std::get_if<JumpInst>(&instruction))
  {
    output_ << "br label %bb" << inst->target_block_id_;
  }
  else if (const auto *inst = std::get_if<BranchInst>(&instruction))
  {
    output_ << "br i1 ";
    printOperand(inst->cond_operand_);
    output_ << ", label %bb" << inst->true_block_id_
            << ", label %bb" << inst->false_block_id_;
  }
  else if (const auto *inst = std::get_if<ReturnInst>(&instruction))
  {
    output_ << "ret ";
    if (inst->return_operand_.has_value())
    {
      const Operand &operand = inst->return_operand_.value();
      printType(getOperandTypeId(operand));
      output_ << ' ';
      printOperand(operand);
    }
    else
    {
      output_ << "void";
    }
  }
  else
  {
    throw std::logic_error("Unknown instruction");
  }
  output_ << '\n';
}
} // namespace ir
