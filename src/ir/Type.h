#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace ir
{

using IRTypeId = std::size_t;

struct VoidIRType
{
};

struct IntegerIRType
{
  unsigned bit_width_;
};

struct PointerIRType
{
};

struct ArrayIRType
{
  IRTypeId element_type_id_;
  std::size_t length_;
};

struct StructIRType
{
  std::string name_;
  std::vector<IRTypeId> fields_;
};

using IRType =
    std::variant<VoidIRType, IntegerIRType, PointerIRType, ArrayIRType, StructIRType>;

} // namespace ir
