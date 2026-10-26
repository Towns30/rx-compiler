#include "TypeResolver.h"
#include <limits>
#include <unordered_set>

std::optional<TypeId>
TypeResolver::resolveType(const Type *type, SemanticContext &semantic_ctx,
                          std::optional<StructId> owner_struct_id)
{
  if (auto *path_type =
          dynamic_cast<const PathType *>(type)) // self会被解析为 PathType
  {
    if (path_type->path_segments_.size() != 1)
    {
      return std::nullopt;
    }
    auto &path_segment = path_type->path_segments_[0];
    if (path_segment.generic_args_.size() == 1) // 有参数，是box或vec
    {
      if (path_segment.name_ != "Box" && path_segment.name_ != "Vec")
      {
        return std::nullopt;
      }
      auto generic_type = resolveType(path_segment.generic_args_[0].type_.get(),
                                      semantic_ctx, owner_struct_id);
      if (generic_type == std::nullopt)
      {
        return std::nullopt;
      }
      if (path_segment.name_ == "Box")
      {
        return semantic_ctx.getOrCreateBoxTypeId(generic_type.value());
      }
      else
      {
        return semantic_ctx.getOrCreateVecTypeId(generic_type.value());
      }
    }
    else if (path_segment.generic_args_.size() == 0)
    {
      if (path_segment.name_ == "Self")
      {
        if (owner_struct_id == std::nullopt)
        {
          return std::nullopt;
        }
        return semantic_ctx.getStructTypeId(owner_struct_id.value());
      }
      auto type_id = semantic_ctx.getBuiltinTypeId(path_segment.name_);
      if (type_id != std::nullopt)
      {
        return type_id;
      }
      type_id = semantic_ctx.getStructTypeId(path_segment.name_);
      return type_id;
    }
    else // 最多也就 box<T>, 两个参数肯定不行
    {
      return std::nullopt;
    }
  }
  else if (auto *reference_type = dynamic_cast<const ReferenceType *>(type))
  {
    auto inner_type = reference_type->inner_type_.get();
    auto inner_type_id = resolveType(inner_type, semantic_ctx, owner_struct_id);
    if (inner_type_id == std::nullopt)
    {
      return std::nullopt;
    }
    return semantic_ctx.getOrCreateReferenceTypeId(inner_type_id.value(),
                                                   reference_type->mut_);
  }
  else if (auto *unit_type = dynamic_cast<const UnitType *>(type))
  {
    return semantic_ctx.getBuiltinTypeId("()");
  }
  else if (auto *array_type = dynamic_cast<const ArrayType *>(type))
  {
    auto element_type_id =
        resolveType(array_type->type_.get(), semantic_ctx, owner_struct_id);
    if (element_type_id == std::nullopt)
    {
      return std::nullopt;
    }
    ConstEvaluator const_evaluator;
    auto const_result = const_evaluator.evaluateExpr(
        semantic_ctx, array_type->const_value_.get(), 3, owner_struct_id);
    if (const_result == std::nullopt)
    {
      return std::nullopt;
    }
    const auto *length = std::get_if<std::int64_t>(&const_result->value_);
    if (!length || *length < 0 ||
        *length > std::numeric_limits<std::uint32_t>::max())
    {
      return std::nullopt;
    }
    return semantic_ctx.getOrCreateArrayTypeId(element_type_id.value(),
                                               *length);
  }
  return std::nullopt;
}

bool TypeResolver::resolveConstTypes(SemanticContext &semantic_ctx)
{
  for (auto &const_info : semantic_ctx.constants_)
  {
    if (const_info.type_id_ != std::nullopt)
    {
      continue;
    }
    auto type = const_info.declaration_->type_.get();
    auto path_type = dynamic_cast<const PathType *>(type);
    if (path_type == nullptr)
    {
      return false;
    }
    if (path_type->path_segments_.size() != 1)
    {
      return false;
    }
    const auto &path_segment = path_type->path_segments_[0];
    if (path_segment.generic_args_.size() != 0)
    {
      return false;
    }
    auto name = path_segment.name_;
    auto type_id = semantic_ctx.getBuiltinTypeId(name);
    if (type_id == std::nullopt || type_id.value() == 5 || type_id.value() == 6)
    {
      return false;
    }
    const_info.type_id_ = type_id;
  }
  return true;
}

bool TypeResolver::resolveFuncStructTypes(SemanticContext &semantic_ctx)
{
  for (auto &func_info : semantic_ctx.functions_)
  {
    const auto &decl = *func_info.declaration_;
    if (decl.self_param_ && !func_info.owner_struct_id_)
    {
      return false;
    }
    func_info.self_param_ = decl.self_param_;
    auto return_type = func_info.declaration_->return_type_.get();
    std::optional<TypeId> return_type_id;
    if (!return_type) // 返回类型为 ()
    {
      return_type_id = 5;
    }
    else
    {
      return_type_id =
          resolveType(return_type, semantic_ctx, func_info.owner_struct_id_);
    }
    if (return_type_id == std::nullopt)
    {
      return false;
    }
    func_info.return_type_id_ = return_type_id.value();
    std::unordered_set<std::string> param_names;
    for (auto &func_param : func_info.declaration_->func_params_)
    {
      auto param_name = func_param.ident_;
      if (!param_names.insert(param_name).second)
      {
        return false;
      }
      auto type = func_param.type_.get();
      auto arg_type_id =
          resolveType(type, semantic_ctx, func_info.owner_struct_id_);
      if (arg_type_id == std::nullopt)
      {
        return false;
      }
      func_info.args_type_id_.push_back(arg_type_id.value());
    }
  }
  for (auto &struct_info : semantic_ctx.structs_)
  {
    std::unordered_set<std::string> field_names;
    for (auto &struct_field : struct_info.declaration_->fields_)
    {
      auto name = struct_field.ident_;
      auto type_id =
          resolveType(struct_field.type_.get(), semantic_ctx,
                      semantic_ctx.struct_name_to_id_.at(struct_info.name_));
      if (type_id == std::nullopt)
      {
        return false;
      }
      if (!field_names.insert(name).second)
      {
        return false;
      }
      struct_info.members_.push_back(
          FieldInfo{.name_ = name, .type_id_ = type_id.value()});
    }
  }
  return true;
}

bool TypeResolver::checkStructCycle(SemanticContext &semantic_ctx)
{
  std::vector<VisitState> states(semantic_ctx.structs_.size(),
                                 VisitState::Unvisited);
  for (int i = 0; i < semantic_ctx.structs_.size(); i++)
  {
    if (!checkStruct(semantic_ctx, states, i))
    {
      return false;
    }
  }
  return true;
}
bool TypeResolver::checkStruct(SemanticContext &semantic_context,
                               std::vector<VisitState> &states,
                               StructId struct_id)
{
  if (states[struct_id] == VisitState::Done)
  {
    return true;
  }
  if (states[struct_id] == VisitState::Visiting)
  {
    return false;
  }
  states[struct_id] = VisitState::Visiting;
  auto struct_info = semantic_context.structs_[struct_id];
  for (auto struct_field : struct_info.members_)
  {
    if (!checkType(semantic_context, states, struct_field.type_id_))
    {
      return false;
    }
  }
  states[struct_id] = VisitState::Done;
  return true;
}
bool TypeResolver::checkType(SemanticContext &semantic_context,
                             std::vector<VisitState> &states, TypeId type_id)
{
  if (type_id <= 6)
  {
    return true;
  }
  const auto &type = semantic_context.getType(type_id);
  if (auto struct_type = dynamic_cast<const StructSemType *>(&type))
  {
    return checkStruct(semantic_context, states, struct_type->struct_id_);
  }
  if (auto array_type = dynamic_cast<const ArraySemType *>(&type))
  {
    return checkType(semantic_context, states, array_type->element_type_id_);
  }
  return true;
}
