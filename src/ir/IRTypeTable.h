#pragma once
#include "Type.h"
#include <optional>

namespace ir
{

class IRTypeTable
{
public:
  IRTypeTable();

  IRType getType(IRTypeId id);
  std::size_t getTypeCount() const;
  IRTypeId getOrCreateVoidType();
  IRTypeId getOrCreateIntegerType(unsigned bit_width);
  IRTypeId getOrCreatePointerType();
  IRTypeId getOrCreateArrayType(IRTypeId element_type, std::size_t length);
  IRTypeId createStructType(StructId struct_id);
  std::optional<IRTypeId> getStructIRTypeId(StructId struct_id) const;

private:
  std::vector<IRType> types_;
};

} // namespace ir
