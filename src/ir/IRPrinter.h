#pragma once

#include "Context.h"
#include "IR.h"
#include <ostream>

namespace ir
{

class IRPrinter
{
public:
  IRPrinter(IRContext &ir_context, std::ostream &output);
  void printIR(const Module &module);
  void printType(IRTypeId type_id);
  void printOperand(const Operand &operand);
  void printInstruction(const Instruction &instruction);

private:
  void printFunction(const Function &function);
  void printFunctionName(FunctionId func_id);

  IRContext &ir_context_;
  std::ostream &output_;
};

} // namespace ir
