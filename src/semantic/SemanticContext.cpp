#include "SemanticContext.h"

const SemType &SemanticContext::getType(TypeId type_id)
{
  return *types_[type_id];
}

bool SemanticContext::checkMain()
{
  auto main_it = func_const_name_to_symbol_.find("main");
  if (main_it == func_const_name_to_symbol_.end() ||
      main_it->second.kind_ != ValueKind::Function)
  {
    return false;
  }
  const auto &main_info = functions_[main_it->second.index_];
  if (!main_info.args_type_id_.empty())
  {
    return false;
  }
  auto return_unit_type = dynamic_cast<const UnitSemType *>(
             &getType(main_info.return_type_id_));
  if (!return_unit_type)
  {
    return false;
  }
  return true;
}

bool SemanticContext::checkDerives()
{
  for (const auto &struct_info : structs_)
  {
    if (struct_info.derives_.empty())
    {
      continue;
    }
    if (!struct_info.type_id_)
    {
      return false;
    }
    TypeId type_id = struct_info.type_id_.value();
    for (std::size_t i = 0; i < struct_info.derives_.size(); i++)
    {
    // 不能重复派生
      for (std::size_t j = 0; j < i; j++)
      {
        if (struct_info.derives_[i] == struct_info.derives_[j])
        {
          return false;
        }
      }
      auto derive_kind = struct_info.derives_[i];
      if (derive_kind == DeriveKind::Copy && !isCopyType(type_id))
      {
        return false;
      }
      if (derive_kind == DeriveKind::Clone && !isCloneType(type_id))
      {
        return false;
      }
      if (derive_kind == DeriveKind::PartialEq && !isPartialEqType(type_id))
      {
        return false;
      }
      if (derive_kind == DeriveKind::Eq && !isEqType(type_id))
      {
        return false;
      }
    }
  }
  return true;
}

bool SemanticContext::isCopyType(TypeId type_id)
{
  const auto &type = getType(type_id);
  if (dynamic_cast<const IntegerSemType *>(&type) ||
      dynamic_cast<const BoolSemType *>(&type) ||
      dynamic_cast<const UnitSemType *>(&type) ||
      dynamic_cast<const NeverSemType *>(&type))
  {
    return true;
  }
  if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&type))
  {
    // 共享引用可以，可变引用不行
    if (reference_type->mut_)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  if (auto array_type = dynamic_cast<const ArraySemType *>(&type))
  {
    return isCopyType(array_type->element_type_id_);
  }
  if (auto struct_type = dynamic_cast<const StructSemType *>(&type))
  {
    const auto &struct_info = structs_[struct_type->struct_id_];
    bool has_copy = false;
    bool has_clone = false;
    for (auto derive_kind : struct_info.derives_)
    {
      if (derive_kind == DeriveKind::Copy)
      {
        has_copy = true;
      }
      else if (derive_kind == DeriveKind::Clone)
      {
        has_clone = true;
      }
    }
    if (!has_copy || !has_clone)
    {
      return false;
    }
    for (const auto &field_info : struct_info.members_)
    {
      if (!isCopyType(field_info.type_id_))
      {
        return false;
      }
    }
    return true;
  }
  // Box和Vec不支持Copy
  return false;
}

bool SemanticContext::isPartialEqType(TypeId type_id)
{
  std::vector<TypeId> checking_type_ids;
  return isPartialEqTypeImpl(type_id, checking_type_ids);
}

bool SemanticContext::isPartialEqTypeImpl(
    TypeId type_id, std::vector<TypeId> &checking_type_ids)
{
  const auto &type = getType(type_id);
  if (dynamic_cast<const IntegerSemType *>(&type) ||
      dynamic_cast<const BoolSemType *>(&type))
  {
    return true;
  }
  if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&type))
  {
    return isPartialEqTypeImpl(reference_type->inner_type_id_, checking_type_ids);
  }
  if (auto array_type = dynamic_cast<const ArraySemType *>(&type))
  {
    return isPartialEqTypeImpl(array_type->element_type_id_, checking_type_ids);
  }
  if (auto box_type = dynamic_cast<const BoxSemType *>(&type))
  {
    return isPartialEqTypeImpl(box_type->element_type_id_, checking_type_ids);
  }
  if (auto vec_type = dynamic_cast<const VecSemType *>(&type))
  {
    return isPartialEqTypeImpl(vec_type->element_type_id_, checking_type_ids);
  }
  if (auto struct_type = dynamic_cast<const StructSemType *>(&type))
  {
    const auto &struct_info = structs_[struct_type->struct_id_];
    bool has_partial_eq = false;
    for (auto derive_kind : struct_info.derives_)
    {
      if (derive_kind == DeriveKind::PartialEq)
      {
        has_partial_eq = true;
        break;
      }
    }
    if (!has_partial_eq)
    {
      return false;
    }
    // 递归环必须经过结构体，为避免无限递归先返回true
    for (auto checking_type_id : checking_type_ids)
    {
      if (checking_type_id == type_id)
      {
        return true;
      }
    }
    // 第一次遇到结构体
    checking_type_ids.push_back(type_id);
    for (const auto &field_info : struct_info.members_)
    {
      if (!isPartialEqTypeImpl(field_info.type_id_, checking_type_ids))
      {
        checking_type_ids.pop_back();
        return false;
      }
    }
    checking_type_ids.pop_back();
    return true;
  }
  // Unit 和 Never 不满足
  return false;
}

bool SemanticContext::isCloneType(TypeId type_id)
{
  std::vector<TypeId> checking_type_ids;
  return isCloneTypeImpl(type_id, checking_type_ids);
}

bool SemanticContext::isCloneTypeImpl(
    TypeId type_id, std::vector<TypeId> &checking_type_ids)
{
  const auto &type = getType(type_id);
  if (dynamic_cast<const IntegerSemType *>(&type) ||
      dynamic_cast<const BoolSemType *>(&type))
  {
    return true;
  }
  if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&type))
  {
    return !reference_type->mut_;
  }
  if (auto array_type = dynamic_cast<const ArraySemType *>(&type))
  {
    return isCloneTypeImpl(array_type->element_type_id_, checking_type_ids);
  }
  if (auto box_type = dynamic_cast<const BoxSemType *>(&type))
  {
    return isCloneTypeImpl(box_type->element_type_id_, checking_type_ids);
  }
  if (auto vec_type = dynamic_cast<const VecSemType *>(&type))
  {
    return isCloneTypeImpl(vec_type->element_type_id_, checking_type_ids);
  }
  if (auto struct_type = dynamic_cast<const StructSemType *>(&type))
  {
    const auto &struct_info = structs_[struct_type->struct_id_];
    bool has_clone = false;
    for (auto derive_kind : struct_info.derives_)
    {
      if (derive_kind == DeriveKind::Clone)
      {
        has_clone = true;
        break;
      }
    }
    if (!has_clone)
    {
      return false;
    }
    // 再次遇到结构体时，暂时满足这条循环依赖
    for (auto checking_type_id : checking_type_ids)
    {
      if (checking_type_id == type_id)
      {
        return true;
      }
    }
    checking_type_ids.push_back(type_id);
    for (const auto &field_info : struct_info.members_)
    {
      if (!isCloneTypeImpl(field_info.type_id_, checking_type_ids))
      {
        checking_type_ids.pop_back();
        return false;
      }
    }
    checking_type_ids.pop_back();
    return true;
  }
  // Unit 和 Never 不行
  return false;
}

bool SemanticContext::isEqType(TypeId type_id)
{
  std::vector<TypeId> checking_type_ids;
  return isEqTypeImpl(type_id, checking_type_ids);
}

bool SemanticContext::isEqTypeImpl(
    TypeId type_id, std::vector<TypeId> &checking_type_ids)
{
  const auto &type = getType(type_id);
  if (dynamic_cast<const IntegerSemType *>(&type) ||
      dynamic_cast<const BoolSemType *>(&type))
  {
    return true;
  }
  if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&type))
  {
    return isEqTypeImpl(reference_type->inner_type_id_, checking_type_ids);
  }
  if (auto array_type = dynamic_cast<const ArraySemType *>(&type))
  {
    return isEqTypeImpl(array_type->element_type_id_, checking_type_ids);
  }
  if (auto box_type = dynamic_cast<const BoxSemType *>(&type))
  {
    return isEqTypeImpl(box_type->element_type_id_, checking_type_ids);
  }
  if (auto vec_type = dynamic_cast<const VecSemType *>(&type))
  {
    return isEqTypeImpl(vec_type->element_type_id_, checking_type_ids);
  }
  if (auto struct_type = dynamic_cast<const StructSemType *>(&type))
  {
    const auto &struct_info = structs_[struct_type->struct_id_];
    bool has_eq = false;
    bool has_partial_eq = false;
    for (auto derive_kind : struct_info.derives_)
    {
      if (derive_kind == DeriveKind::Eq)
      {
        has_eq = true;
      }
      else if (derive_kind == DeriveKind::PartialEq)
      {
        has_partial_eq = true;
      }
    }
    if (!has_eq || !has_partial_eq)
    {
      return false;
    }
    for (auto checking_type_id : checking_type_ids)
    {
      if (checking_type_id == type_id)
      {
        // 暂时放过
        return true;
      }
    }
    checking_type_ids.push_back(type_id);
    for (const auto &field_info : struct_info.members_)
    {
      if (!isEqTypeImpl(field_info.type_id_, checking_type_ids))
      {
        checking_type_ids.pop_back();
        return false;
      }
    }
    checking_type_ids.pop_back();
    return true;
  }
  // Unit 和 Never 不行
  return false;
}

void SemanticContext::addBuiltinType(const std::string &name,
                                     std::unique_ptr<SemType> type)
{
  if (builtin_type_to_type_id_.find(name) != builtin_type_to_type_id_.end())
  {
    return;
  }
  const TypeId type_id = types_.size();
  types_.push_back(std::move(type));
  builtin_type_to_type_id_.emplace(name, type_id);
}

void SemanticContext::addStructType(const std::string &name,
                                    std::unique_ptr<SemType> type)
{
  if (struct_name_to_id_.find(name) == struct_name_to_id_.end())
  {
    return;
  }
  StructId struct_id = struct_name_to_id_.find(name)->second;
  if (struct_id_to_type_id_.find(struct_id) != struct_id_to_type_id_.end())
  {
    return;
  }
  const TypeId type_id = types_.size();
  types_.push_back(std::move(type));
  struct_id_to_type_id_.emplace(struct_id, type_id);
}

void SemanticContext::initBuiltinType()
{
  addBuiltinType("i32", std::make_unique<IntegerSemType>(IntegerType::I32));
  addBuiltinType("u32", std::make_unique<IntegerSemType>(IntegerType::U32));
  addBuiltinType("isize", std::make_unique<IntegerSemType>(IntegerType::ISize));
  addBuiltinType("usize", std::make_unique<IntegerSemType>(IntegerType::USize));
  addBuiltinType("bool", std::make_unique<BoolSemType>());
  addBuiltinType("()", std::make_unique<UnitSemType>());
  addBuiltinType("!", std::make_unique<NeverSemType>());
}

void SemanticContext::addBuiltinFunction(
    const std::string &name, const std::vector<TypeId> &args_type_id,
    TypeId return_type_id)
{
  if (func_const_name_to_symbol_.find(name) !=
      func_const_name_to_symbol_.end())
  {
    return;
  }
  std::size_t func_id = functions_.size();
  functions_.push_back(FuncInfo{.name_ = name,
                                .args_type_id_ = args_type_id,
                                .return_type_id_ = return_type_id,
                                .is_builtin_ = true});
  func_const_name_to_symbol_.emplace(
      name, FuncConstSymbol{.kind_ = ValueKind::Function, .index_ = func_id});
}

void SemanticContext::initBuiltinFunctions()
{
  TypeId i32_type_id = getBuiltinTypeId("i32").value();
  TypeId unit_type_id = getBuiltinTypeId("()").value();
  addBuiltinFunction("get_i32", {}, i32_type_id);
  addBuiltinFunction("print_i32", {i32_type_id}, unit_type_id);
  addBuiltinFunction("println_i32", {i32_type_id}, unit_type_id);
}

void SemanticContext::initStructType()
{
  for (auto &struct_info : structs_)
  {
    StructId struct_id = struct_name_to_id_.find(struct_info.name_)->second;
    addStructType(struct_info.name_,
                  std::make_unique<StructSemType>(struct_id));
    struct_info.type_id_ = struct_id_to_type_id_.find(struct_id)->second;
  }
}

std::optional<TypeId> SemanticContext::getBuiltinTypeId(const std::string &name)
{
  auto it = builtin_type_to_type_id_.find(name);
  if (it == builtin_type_to_type_id_.end())
  {
    return std::nullopt;
  }
  return it->second;
}

std::optional<TypeId> SemanticContext::getStructTypeId(const std::string &name)
{
  auto it = struct_name_to_id_.find(name);
  if (it == struct_name_to_id_.end())
  {
    return std::nullopt;
  }
  return getStructTypeId(it->second);
}

std::optional<TypeId>
SemanticContext::getStructTypeId(const StructId &struct_id)
{
  auto it = struct_id_to_type_id_.find(struct_id);
  if (it == struct_id_to_type_id_.end())
  {
    return std::nullopt;
  }
  return it->second;
}

TypeId SemanticContext::getOrCreateReferenceTypeId(TypeId inner_type_id, bool mut)
{
  ReferenceSemType reference_type(inner_type_id, mut);
  if (auto type_id = getReferenceTypeId(reference_type))
  {
    return type_id.value();
  }
  TypeId new_type_id = types_.size();
  types_.push_back(std::make_unique<ReferenceSemType>(reference_type));
  return new_type_id;
}

TypeId SemanticContext::getOrCreateArrayTypeId(TypeId element_type_id,
                                             std::int64_t len)
{
  ArraySemType array_type(element_type_id, len);
  if (auto type_id = getArrayTypeId(array_type))
  {
    return type_id.value();
  }
  TypeId new_type_id = types_.size();
  types_.push_back(std::make_unique<ArraySemType>(array_type));
  return new_type_id;
}

TypeId SemanticContext::getOrCreateBoxTypeId(TypeId element_type_id) 
{
  BoxSemType box_type(element_type_id);
  if (auto type_id = getBoxTypeId(box_type))
  {
    return type_id.value();
  }
  TypeId new_type_id = types_.size();
  types_.push_back(std::make_unique<BoxSemType>(box_type));
  return new_type_id;
}

TypeId SemanticContext::getOrCreateVecTypeId(TypeId element_type_id) 
{
  VecSemType vec_type(element_type_id);
  if (auto type_id = getVecTypeId(vec_type))
  {
    return type_id.value();
  }
  TypeId new_type_id = types_.size();
  types_.push_back(std::make_unique<VecSemType>(vec_type));
  return new_type_id;
}

std::optional<TypeId> SemanticContext::getReferenceTypeId(const ReferenceSemType &reference_type) 
{
  for (TypeId i = 0; i < types_.size(); i++)
  {
    auto &type = types_[i];
    if (auto store_reference_type = dynamic_cast<const ReferenceSemType*>(type.get()))
    {
      if (store_reference_type->inner_type_id_ == reference_type.inner_type_id_ && store_reference_type->mut_ == reference_type.mut_)
      {
        return i;
      }
    }
  }
  return std::nullopt;
}
std::optional<TypeId> SemanticContext::getArrayTypeId(const ArraySemType &array_type)
{
  for (TypeId i = 0; i < types_.size(); i++)
  {
    auto &type = types_[i];
    if (auto store_array_type = dynamic_cast<const ArraySemType*>(type.get()))
    {
      if (store_array_type->element_type_id_ == array_type.element_type_id_ && store_array_type->length_ == array_type.length_)
      {
        return i;
      }
    }
  }
  return std::nullopt;
}
std::optional<TypeId> SemanticContext::getBoxTypeId(const BoxSemType &box_type) 
{
  for (TypeId i = 0; i < types_.size(); i++)
  {
    auto &type = types_[i];
    if (auto store_box_type = dynamic_cast<const BoxSemType*>(type.get()))
    {
      if (store_box_type->element_type_id_ == box_type.element_type_id_)
      {
        return i;
      }
    }
  }
  return std::nullopt;
}
std::optional<TypeId> SemanticContext::getVecTypeId(const VecSemType &vec_type) 
{
  for (TypeId i = 0; i < types_.size(); i++)
  {
    auto &type = types_[i];
    if (auto store_vec_type = dynamic_cast<const VecSemType*>(type.get()))
    {
      if (store_vec_type->element_type_id_ == vec_type.element_type_id_)
      {
        return i;
      }
    }
  }
  return std::nullopt;
}
