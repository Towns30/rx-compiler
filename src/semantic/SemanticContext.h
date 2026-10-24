#pragma once

#include "Symbols.h"
#include "Types.h"
#include <unordered_map>
#include <cstdint>

class SemanticContext
{
public:
  const SemType &getType(TypeId type_id);
  void initBuiltinType();
  void initStructType();
  std::optional<TypeId> getBuiltinTypeId(const std::string& name);
  std::optional<TypeId> getStructTypeId(const std::string& name);
  std::optional<TypeId> getStructTypeId(const StructId& struct_id);
  TypeId getOrCreateReferenceTypeId(TypeId inner_type_id, bool mut);
  TypeId getOrCreateArrayTypeId(TypeId element_type_id, std::uint32_t len);
  TypeId getOrCreateBoxTypeId(TypeId element_type_id);
  TypeId getOrCreateVecTypeId(TypeId element_type_id);

  std::vector<std::unique_ptr<SemType>> types_;
  std::vector<StructInfo> structs_;
  std::vector<FuncInfo> functions_;
  std::vector<ConstInfo> constants_;
  std::unordered_map<std::string, StructId> struct_name_to_id_;
  std::unordered_map<std::string, FuncConstSymbol> func_const_name_to_id;
  std::unordered_map<std::string, TypeId> builtin_type_to_type_id_;
  std::unordered_map<StructId, TypeId> struct_id_to_type_id_;

private:
  void addBuiltinType(const std::string &name, std::unique_ptr<SemType> type);
  void addStructType(const std::string &name, std::unique_ptr<SemType> type);
  std::optional<TypeId> getReferenceTypeId(const ReferenceSemType& reference_type);
  std::optional<TypeId> getArrayTypeId(const ArraySemType& array_type);
  std::optional<TypeId> getBoxTypeId(const BoxSemType& box_type);
  std::optional<TypeId> getVecTypeId(const VecSemType& vec_type);

};
