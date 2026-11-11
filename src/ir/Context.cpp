#include "Context.h"
#include <cassert>
#include <utility>

namespace ir
{

IRContext::IRContext()
{
  types_.push_back(VoidIRType{}); // void 0
  types_.push_back(PointerIRType{}); // ptr 1
}

IRType IRContext::getType(IRTypeId id)
{
  return types_.at(id);
}

std::size_t IRContext::getTypeCount() const
{
  return types_.size();
}

IRTypeId IRContext::getOrCreateVoidType()
{
  return 0;
}

IRTypeId IRContext::getOrCreateIntegerType(unsigned bit_width)
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

IRTypeId IRContext::getOrCreatePointerType()
{
  return 1;
}

IRTypeId IRContext::getOrCreateArrayType(IRTypeId element_type, std::size_t length)
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

IRTypeId IRContext::createStructType(std::string name)
{
  IRTypeId type_id = types_.size();
  types_.push_back(StructIRType{.name_ = std::move(name), .fields_ = {}});
  return type_id;
}

void IRContext::setStructFields(IRTypeId id, std::vector<IRTypeId> fields)
{
  auto &struct_type = std::get<StructIRType>(types_.at(id));
  struct_type.fields_ = std::move(fields);
}

} // namespace ir
