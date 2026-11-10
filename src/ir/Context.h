#pragma once
#include "Type.h"

namespace ir
{

class IRContext
{
public:
  IRContext();

  IRType getType(IRTypeId id);
  std::size_t getTypeCount() const;
  IRTypeId getVoidType();
  IRTypeId getIntegerType(unsigned bit_width);
  IRTypeId getPointerType();
  IRTypeId getArrayType(IRTypeId element_type, std::size_t length);
  IRTypeId createStructType(std::string name);
  void setStructFields(IRTypeId id, std::vector<IRTypeId> fields);

private:
  std::vector<IRType> types_;
};

}
