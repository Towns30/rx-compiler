#pragma once

#include "Symbols.h"
#include "Types.h"
#include <unordered_map>
#include <cstdint>

class SemanticContext
{
public:
  const SemType &getType(TypeId type_id);
  bool isCopyType(TypeId type_id);
  bool isPartialEqType(TypeId type_id);
  bool isCloneType(TypeId type_id);
  bool isEqType(TypeId type_id);
  bool checkDerives();
  bool checkMain();
  void initBuiltinType();
  void initBuiltinFunctions();
  void initStructType();
  std::optional<TypeId> getBuiltinTypeId(const std::string& name);
  std::optional<TypeId> getStructTypeId(const std::string& name);
  std::optional<TypeId> getStructTypeId(const StructId& struct_id);
  TypeId getOrCreateReferenceTypeId(TypeId inner_type_id, bool mut);
  TypeId getOrCreateArrayTypeId(TypeId element_type_id, std::int64_t len);
  TypeId getOrCreateBoxTypeId(TypeId element_type_id);
  TypeId getOrCreateVecTypeId(TypeId element_type_id);

  std::vector<std::unique_ptr<SemType>> types_;
  std::vector<StructInfo> structs_;
  std::vector<FuncInfo> functions_;
  std::vector<ConstInfo> constants_;
  std::unordered_map<std::string, StructId> struct_name_to_id_;
  std::unordered_map<std::string, FuncConstSymbol> func_const_name_to_symbol_;
  std::unordered_map<std::string, TypeId> builtin_type_to_type_id_;
  std::unordered_map<StructId, TypeId> struct_id_to_type_id_;

private:
  bool isPartialEqTypeImpl(TypeId type_id,
                           std::vector<TypeId> &checking_type_ids);
  bool isCloneTypeImpl(TypeId type_id,
                       std::vector<TypeId> &checking_type_ids);
  bool isEqTypeImpl(TypeId type_id,
                    std::vector<TypeId> &checking_type_ids);
  void addBuiltinType(const std::string &name, std::unique_ptr<SemType> type);
  void addBuiltinFunction(const std::string &name,
                          const std::vector<TypeId> &args_type_id,
                          TypeId return_type_id);
  void addStructType(const std::string &name, std::unique_ptr<SemType> type);
  std::optional<TypeId> getReferenceTypeId(const ReferenceSemType& reference_type);
  std::optional<TypeId> getArrayTypeId(const ArraySemType& array_type);
  std::optional<TypeId> getBoxTypeId(const BoxSemType& box_type);
  std::optional<TypeId> getVecTypeId(const VecSemType& vec_type);

};
