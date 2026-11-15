#pragma once

#include "../Ids.h"
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
  StructId struct_id_;
};

struct VecIRType
{
  // 按 data、len、capacity 三个字段存储。
  IRTypeId element_type_id_;
};

using IRType = std::variant<VoidIRType, IntegerIRType, PointerIRType, ArrayIRType, StructIRType, VecIRType>;

} // namespace ir
