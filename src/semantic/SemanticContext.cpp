#include "SemanticContext.h"

const SemType &SemanticContext::getType(TypeId type_id)
{
  return *types_[type_id];
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
