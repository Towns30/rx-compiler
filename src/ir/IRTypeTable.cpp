#include "IRTypeTable.h"
#include <cassert>
#include <utility>

namespace ir
{

IRTypeTable::IRTypeTable()
{
  types_.push_back(VoidIRType{}); // void 0
  types_.push_back(PointerIRType{}); // ptr 1
}

IRType IRTypeTable::getType(IRTypeId id)
{
  return types_.at(id);
}

std::size_t IRTypeTable::getTypeCount() const
{
  return types_.size();
}

IRTypeId IRTypeTable::getOrCreateVoidType()
{
  return 0;
}

IRTypeId IRTypeTable::getOrCreateIntegerType(unsigned bit_width)
{
  for (std::size_t i = 0; i < types_.size(); i++)
  {
    const auto *integer_type = std::get_if<IntegerIRType>(&types_[i]);
    if (!integer_type)
    {
      continue;
    }
    if (integer_type->bit_width_ == bit_width)
    {
      return i;
    }
  }
  IRTypeId type_id = types_.size();
  types_.push_back(IntegerIRType{.bit_width_ = bit_width});
  return type_id;
}

IRTypeId IRTypeTable::getOrCreatePointerType()
{
  return 1;
}

IRTypeId IRTypeTable::getOrCreateArrayType(IRTypeId element_type, std::size_t length)
{
  for (std::size_t i = 0; i < types_.size(); i++)
  {
    const auto *array_type = std::get_if<ArrayIRType>(&types_[i]);
    if (!array_type)
    {
      continue;
    }
    if (array_type->element_type_id_ == element_type &&
        array_type->length_ == length)
    {
      return i;
    }
  }
  IRTypeId type_id = types_.size();
  types_.push_back(ArrayIRType{.element_type_id_ = element_type, .length_ = length});
  return type_id;
}

IRTypeId IRTypeTable::createStructType(StructId struct_id)
{
  IRTypeId type_id = types_.size();
  types_.push_back(StructIRType{.struct_id_ = struct_id});
  return type_id;
}

std::optional<IRTypeId> IRTypeTable::getStructIRTypeId(StructId struct_id) const
{
  for (IRTypeId type_id = 0; type_id < types_.size(); type_id++)
  {
    const auto *struct_type = std::get_if<StructIRType>(&types_[type_id]);
    if (!struct_type)
    {
      continue;
    }
    if (struct_type->struct_id_ == struct_id)
    {
      return type_id;
    }
  }
  return std::nullopt;
}

} // namespace ir
