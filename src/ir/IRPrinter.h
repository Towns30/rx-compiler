#pragma once

#include "IRSymbol.h"
#include "IRTypeTable.h"
#include <ostream>

namespace ir
{

class IRPrinter
{
public:
  IRPrinter(IRTypeTable &ir_type_table, std::ostream &output);
  void printIR(const Module &module);
  void printType(IRTypeId type_id);
  void printOperand(const Operand &operand);
  void printInstruction(const Instruction &instruction);

private:
  void printFunction(const Function &function);
  void printFunctionName(FunctionId func_id);

  IRTypeTable &ir_type_table_;
  std::ostream &output_;
};

} // namespace ir
