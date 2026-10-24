#pragma once
#include "../Utils.h"
#include "../ast/Ast.h"

using StructId = std::size_t;
using TypeId = std::size_t;

enum class TypeKind
{
  Integer,
  Bool,
  Unit,
  Never,
  Struct,
  Reference,
  Array,
  Box,
  Vec
};

class SemType
{
public:
  virtual ~SemType() = default;
  virtual TypeKind kind() const = 0;
};


class IntegerSemType : public SemType
{
public:
  IntegerType integer_type_;

  explicit IntegerSemType(IntegerType type) : integer_type_(type) {}

  TypeKind kind() const override { return TypeKind::Integer; }
};

class BoolSemType : public SemType
{
public:
  BoolSemType() = default;

  TypeKind kind() const override { return TypeKind::Bool; }
};

class UnitSemType : public SemType
{
public:
  UnitSemType() = default;

  TypeKind kind() const override { return TypeKind::Unit; }
};

class NeverSemType : public SemType
{
public:
  NeverSemType() = default;

  TypeKind kind() const override { return TypeKind::Never; }
};

class StructSemType : public SemType
{
public:
  StructId struct_id_;

  explicit StructSemType(StructId id) : struct_id_(id) {}

  TypeKind kind() const override { return TypeKind::Struct; }
};

class ReferenceSemType : public SemType
{
public:
  TypeId inner_type_id_;
  bool mut_;

  explicit ReferenceSemType(TypeId inner_type_id, bool mut = false)
      : inner_type_id_(inner_type_id), mut_(mut) {}

  TypeKind kind() const override { return TypeKind::Reference; }
};

class ArraySemType : public SemType
{
public:
  TypeId element_type_id_;
  std::uint32_t length_;

  ArraySemType(TypeId element_type_id, std::uint32_t length)
      : element_type_id_(element_type_id), length_(length) {}

  TypeKind kind() const override { return TypeKind::Array; }
};

class BoxSemType : public SemType
{
public:
  TypeId element_type_id_;

  explicit BoxSemType(TypeId element_type_id)
      : element_type_id_(element_type_id) {}

  TypeKind kind() const override { return TypeKind::Box; }
};

class VecSemType : public SemType
{
public:
  TypeId element_type_id_;

  explicit VecSemType(TypeId element_type_id)
      : element_type_id_(element_type_id) {}

  TypeKind kind() const override { return TypeKind::Vec; }
};
