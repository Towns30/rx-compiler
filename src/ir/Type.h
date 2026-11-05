#pragma once

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace ir
{

struct TypeId
{
  std::size_t index_;

  bool operator==(const TypeId &) const = default;
};

struct VoidType
{
};

struct IntegerType
{
  unsigned bit_width_;
};

struct PointerType
{
};

struct ArrayType
{
  TypeId element_type_id_;
  std::size_t length_;
};

struct StructType
{
  std::string name_;
  std::vector<TypeId> fields_;
};

using Type =
    std::variant<VoidType, IntegerType, PointerType, ArrayType, StructType>;

} // namespace ir
