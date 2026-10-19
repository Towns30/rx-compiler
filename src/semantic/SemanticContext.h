#pragma once

#include "Types.h"
#include "Symbols.h"
#include <unordered_map>

class SemanticContext
{
public:
  std::vector<std::unique_ptr<SemType>> types_;
  std::vector<StructInfo> structs_;
  std::vector<FuncInfo> functions_;
  std::vector<ConstInfo> constants_;
  std::unordered_map<std::string, StructId> struct_ident_to_id_;
  std::unordered_map<std::string, FuncConstSymbol> func_const_ident_to_id;
  

  const SemType& getType(TypeId type_id);
};