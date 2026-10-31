#include "BodyChecker.h"
#include "ConstEvaluator.h"
#include <limits>
#include <unordered_set>
#include <utility>

bool BodyChecker::checkAll(const Crate *crate, SemanticContext &semantic_ctx)
{
  for (auto func_info : semantic_ctx.functions_)
  {
    if (func_info.is_builtin_)
    {
      continue;
    }
    if (!checkFunction(func_info, semantic_ctx))
    {
      return false;
    }
  }
  return true;
}

std::optional<std::size_t> BodyChecker::getLoopTarget()
{
  if (loop_stack_.empty())
  {
    return std::nullopt;
  }
  if (loop_stack_.back().kind_ ==
      LoopKind::WhileCondition) // 不能在condition里跳出循环
  {
    return std::nullopt;
  }
  return loop_stack_.size() - 1;
}

bool BodyChecker::checkFunction(const FuncInfo &func_info,
                                SemanticContext &semantic_ctx)
{
  loop_stack_.clear();
  Scope scope;
  scope.enterScope();
  for (int i = 0; i < func_info.args_type_id_.size(); i++)
  {
    TypeId var_type_id = func_info.args_type_id_[i];
    bool var_mut = func_info.declaration_->func_params_[i].mut_;
    std::string var_name = func_info.declaration_->func_params_[i].ident_;
    scope.bind(
        VarInfo{.name_ = var_name, .type_id_ = var_type_id, .mut_ = var_mut});
  }
  if (func_info.self_param_)
  {
    if (!func_info.owner_struct_id_)
    {
      return false;
    }
    auto self_type_id =
        semantic_ctx.getStructTypeId(func_info.owner_struct_id_.value());
    if (!self_type_id)
    {
      return false;
    }
    const auto &self_param = func_info.self_param_.value();
    TypeId var_type_id;
    bool var_mut;
    if (!self_param.is_reference_ && !self_param.mut_) // self
    {
      var_type_id = self_type_id.value();
      var_mut = false;
    }
    else if (!self_param.is_reference_ && self_param.mut_) // mut self
    {
      var_type_id = self_type_id.value();
      var_mut = true;
    }
    else if (self_param.is_reference_ && !self_param.mut_) // &self
    {
      var_type_id =
          semantic_ctx.getOrCreateReferenceTypeId(self_type_id.value(), false);
      var_mut = false;
    }
    else // &mut self
    {
      var_type_id =
          semantic_ctx.getOrCreateReferenceTypeId(self_type_id.value(), true);
      var_mut = false;
    }
    scope.bind(
        VarInfo{.name_ = "self", .type_id_ = var_type_id, .mut_ = var_mut});
  }
  auto ExprInfo =
      checkBlockExpr(func_info.declaration_->block_expr_.get(), semantic_ctx,
                     scope, func_info.return_type_id_, func_info);
  if (!ExprInfo)
  {
    return false;
  }
  return true;
}

std::optional<ExprInfo>
BodyChecker::checkExpr(const Expr *expr, SemanticContext &semantic_ctx,
                       Scope &scope, std::optional<TypeId> expected_type_id,
                       const FuncInfo &func_info)
{
  if (auto literal_expr = dynamic_cast<const LiteralExpr *>(expr))
  {
    return checkLiteralExpr(literal_expr, semantic_ctx, scope, expected_type_id,
                            func_info);
  }
  if (auto unit_expr = dynamic_cast<const UnitExpr *>(expr))
  {
    return checkUnitExpr(unit_expr, semantic_ctx, scope, expected_type_id,
                         func_info);
  }
  if (auto path_expr = dynamic_cast<const PathExpr *>(expr))
  {
    return checkPathExpr(path_expr, semantic_ctx, scope, expected_type_id,
                         func_info);
  }
  if (auto array_expr = dynamic_cast<const ArrayExpr *>(expr))
  {
    return checkArrayExpr(array_expr, semantic_ctx, scope, expected_type_id,
                          func_info);
  }
  if (auto struct_expr = dynamic_cast<const StructExpr *>(expr))
  {
    return checkStructExpr(struct_expr, semantic_ctx, scope, expected_type_id,
                           func_info);
  }
  if (auto block_expr = dynamic_cast<const BlockExpr *>(expr))
  {
    return checkBlockExpr(block_expr, semantic_ctx, scope, expected_type_id,
                          func_info);
  }
  if (auto if_expr = dynamic_cast<const IfExpr *>(expr))
  {
    return checkIfExpr(if_expr, semantic_ctx, scope, expected_type_id,
                       func_info);
  }
  if (auto loop_expr = dynamic_cast<const LoopExpr *>(expr))
  {
    return checkLoopExpr(loop_expr, semantic_ctx, scope, expected_type_id,
                         func_info);
  }
  if (auto while_expr = dynamic_cast<const WhileExpr *>(expr))
  {
    return checkWhileExpr(while_expr, semantic_ctx, scope, expected_type_id,
                          func_info);
  }
  if (auto break_expr = dynamic_cast<const BreakExpr *>(expr))
  {
    return checkBreakExpr(break_expr, semantic_ctx, scope, expected_type_id,
                          func_info);
  }
  if (auto return_expr = dynamic_cast<const ReturnExpr *>(expr))
  {
    return checkReturnExpr(return_expr, semantic_ctx, scope, expected_type_id,
                           func_info);
  }
  if (auto continue_expr = dynamic_cast<const ContinueExpr *>(expr))
  {
    return checkContinueExpr(continue_expr, semantic_ctx, scope,
                             expected_type_id, func_info);
  }
  if (auto unary_expr = dynamic_cast<const UnaryExpr *>(expr))
  {
    return checkUnaryExpr(unary_expr, semantic_ctx, scope, expected_type_id,
                          func_info);
  }
  if (auto binary_expr = dynamic_cast<const BinaryExpr *>(expr))
  {
    return checkBinaryExpr(binary_expr, semantic_ctx, scope, expected_type_id,
                           func_info);
  }
  if (auto assign_expr = dynamic_cast<const AssignExpr *>(expr))
  {
    return checkAssignExpr(assign_expr, semantic_ctx, scope, expected_type_id,
                           func_info);
  }
  if (auto cast_expr = dynamic_cast<const CastExpr *>(expr))
  {
    return checkCastExpr(cast_expr, semantic_ctx, scope, expected_type_id,
                         func_info);
  }
  if (auto call_expr = dynamic_cast<const CallExpr *>(expr))
  {
    return checkCallExpr(call_expr, semantic_ctx, scope, expected_type_id,
                         func_info);
  }
  if (auto index_expr = dynamic_cast<const IndexExpr *>(expr))
  {
    return checkIndexExpr(index_expr, semantic_ctx, scope, expected_type_id,
                          func_info);
  }
  if (auto member_expr = dynamic_cast<const MemberExpr *>(expr))
  {
    return checkMemberExpr(member_expr, semantic_ctx, scope, expected_type_id,
                           func_info);
  }
  if (auto method_call_expr = dynamic_cast<const MethodCallExpr *>(expr))
  {
    return checkMethodCallExpr(method_call_expr, semantic_ctx, scope,
                               expected_type_id, func_info);
  }
  return std::nullopt;
}

std::optional<DerefInfo>
BodyChecker::dereferenceOnce(TypeId type_id, SemanticContext &semantic_ctx)
{
  const auto &type = semantic_ctx.getType(type_id);
  if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&type))
  {
    return DerefInfo{.type_id_ = reference_type->inner_type_id_,
                     .through_shared_ = !reference_type->mut_};
  }
  if (auto box_type = dynamic_cast<const BoxSemType *>(&type))
  {
    return DerefInfo{.type_id_ = box_type->element_type_id_,
                     .through_shared_ = false};
  }
  return std::nullopt;
}

std::optional<MethodInfo>
BodyChecker::findMethod(TypeId receiver_type_id, const std::string &method_name,
                        SemanticContext &semantic_ctx)
{
  const auto &receiver_type = semantic_ctx.getType(receiver_type_id);
  auto reference_type = dynamic_cast<const ReferenceSemType *>(&receiver_type);
  TypeId owner_type_id;
  // &(mut)T
  if (reference_type)
  {
    owner_type_id = reference_type->inner_type_id_;
  }
  else
  {
    owner_type_id = receiver_type_id;
  }
  const auto &owner_type = semantic_ctx.getType(owner_type_id);
  // struct
  if (auto struct_type = dynamic_cast<const StructSemType *>(&owner_type))
  {
    const auto &struct_info = semantic_ctx.structs_[struct_type->struct_id_];
    auto item_it = struct_info.associate_items_.find(method_name);
    if (item_it != struct_info.associate_items_.end() &&
        item_it->second.kind_ == ValueKind::Function)
    {
      const auto &method_info = semantic_ctx.functions_[item_it->second.index_];
      // 检查self参数
      if (method_info.self_param_)
      {
        const auto &self_param = method_info.self_param_.value();
        TypeId declared_receiver_type_id;
        if (self_param.is_reference_)
        {
          declared_receiver_type_id = semantic_ctx.getOrCreateReferenceTypeId(
              owner_type_id, self_param.mut_);
        }
        else
        {
          // self 与 mut self 的接收者类型相同
          declared_receiver_type_id = owner_type_id;
        }
        if (declared_receiver_type_id == receiver_type_id)
        {
          return MethodInfo{.receiver_type_id_ = receiver_type_id,
                            .args_type_id_ = method_info.args_type_id_,
                            .return_type_id_ = method_info.return_type_id_};
        }
      }
    }
  }
  if (!reference_type)
  {
    return std::nullopt;
  }
  // 内置方法，类型必须完全匹配，自动借用调整类型是外层负责
  if (!reference_type->mut_)
  {
    if (method_name == "len" &&
        (dynamic_cast<const ArraySemType *>(&owner_type) ||
         dynamic_cast<const VecSemType *>(&owner_type)))
    {
      return MethodInfo{.receiver_type_id_ = receiver_type_id,
                        .args_type_id_ = {},
                        .return_type_id_ = 3};
    }
    if (method_name == "is_empty" &&
        dynamic_cast<const VecSemType *>(&owner_type))
    {
      return MethodInfo{.receiver_type_id_ = receiver_type_id,
                        .args_type_id_ = {},
                        .return_type_id_ = 4};
    }
    if (method_name == "clone" && semantic_ctx.isCloneType(owner_type_id))
    {
      // 候选是 &T，Clone 查询的是 T，返回类型也是 T
      return MethodInfo{.receiver_type_id_ = receiver_type_id,
                        .args_type_id_ = {},
                        .return_type_id_ = owner_type_id};
    }
  }
  else if (auto vec_type = dynamic_cast<const VecSemType *>(&owner_type))
  {
    if (method_name == "push")
    {
      return MethodInfo{.receiver_type_id_ = receiver_type_id,
                        .args_type_id_ = {vec_type->element_type_id_},
                        .return_type_id_ = 5};
    }
    if (method_name == "remove")
    {
      return MethodInfo{.receiver_type_id_ = receiver_type_id,
                        .args_type_id_ = {3},
                        .return_type_id_ = vec_type->element_type_id_};
    }
  }
  return std::nullopt;
}

bool BodyChecker::canCoerce(TypeId source_type_id, TypeId target_type_id,
                            SemanticContext &semantic_ctx)
{
  if (source_type_id == target_type_id)
  {
    return true;
  }
  if (source_type_id == 6)
  {
    return true;
  }
  auto &source_type = semantic_ctx.getType(source_type_id);
  auto &target_type = semantic_ctx.getType(target_type_id);
  if (!dynamic_cast<const ReferenceSemType *>(&source_type) ||
      !dynamic_cast<const ReferenceSemType *>(&target_type))
  {
    return false;
  }
  const auto *source_ref = dynamic_cast<const ReferenceSemType *>(&source_type);
  const auto *target_ref = dynamic_cast<const ReferenceSemType *>(&target_type);
  if (target_ref->mut_) // &mut T
  {
    while (true)
    {
      auto dere_info = dereferenceOnce(source_type_id, semantic_ctx);
      if (!dere_info || dere_info.value().through_shared_)
      {
        return false;
      }
      if (dere_info.value().type_id_ == target_ref->inner_type_id_)
      {
        return true;
      }
      source_type_id = dere_info.value().type_id_;
    }
  }
  else // & T
  {
    while (true)
    {
      auto dere_info = dereferenceOnce(source_type_id, semantic_ctx);
      if (!dere_info)
      {
        return false;
      }
      if (dere_info.value().type_id_ == target_ref->inner_type_id_)
      {
        return true;
      }
      source_type_id = dere_info.value().type_id_;
    }
  }
}

std::optional<TypeId>
BodyChecker::getLubTypeId(const std::vector<ExprInfo> &expr_infos,
                          SemanticContext &semantic_ctx)
{
  std::optional<TypeId> result_type_id = std::nullopt;
  for (std::size_t i = 0; i < expr_infos.size(); i++)
  {
    TypeId type_id = expr_infos[i].type_id_;
    if (type_id == 6) // never 不约束共同类型
    {
      continue;
    }
    if (!result_type_id) // 没确定先确定一个
    {
      result_type_id = type_id;
    }
    else if (canCoerce(type_id, result_type_id.value(), semantic_ctx))
    {
      continue;
    }
    else if (canCoerce(result_type_id.value(), type_id, semantic_ctx))
    {
      // 切换目标前，重新检查所有更早的结果
      for (std::size_t j = 0; j < i; j++)
      {
        if (!canCoerce(expr_infos[j].type_id_, type_id, semantic_ctx))
        {
          return std::nullopt;
        }
      }
      result_type_id = type_id;
    }
    else
    {
      return std::nullopt;
    }
  }
  if (result_type_id)
  {
    return result_type_id.value();
  }
  else
  {
    return 6; // 所有都是never
  }
}

std::optional<TypeId> BodyChecker::getIntegerTypeId(IntegerType integer_type)
{
  switch (integer_type)
  {
  case IntegerType::I32:
    return 0;
  case IntegerType::U32:
    return 1;
  case IntegerType::ISize:
    return 2;
  case IntegerType::USize:
    return 3;
  default:
    return std::nullopt;
  }
}

std::optional<ExprInfo>
BodyChecker::checkLiteralExpr(const LiteralExpr *literal_expr,
                              SemanticContext &semantic_ctx, Scope &scope,
                              std::optional<TypeId> expected_type_id,
                              const FuncInfo &func_info)
{
  if (literal_expr->is_int_)
  {
    auto type_id = getIntegerTypeId(literal_expr->int_type_);
    if (!type_id)
    {
      if (expected_type_id)
      {
        if (expected_type_id.value() != 0 && expected_type_id.value() != 1 &&
            expected_type_id.value() != 2 && expected_type_id.value() != 3)
        {
          return std::nullopt;
        }
        type_id = expected_type_id;
      }
      else
      {
        type_id = 0; // 无期望也无推断，为i32
      }
    }
    else
    {
      if (expected_type_id && expected_type_id.value() != type_id.value())
      {
        return std::nullopt;
      }
    }
    return ExprInfo{
        .type_id_ = type_id.value(), .is_place_ = false, .mut_ = false};
  }
  else
  {
    if (expected_type_id && expected_type_id.value() != 4)
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = 4, .is_place_ = false, .mut_ = false};
  }
}

std::optional<ExprInfo> BodyChecker::checkUnitExpr(
    const UnitExpr *unit_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  if (expected_type_id && expected_type_id.value() != 5)
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = 5, .is_place_ = false, .mut_ = false};
}

std::optional<ExprInfo> BodyChecker::checkArrayExpr(
    const ArrayExpr *array_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  std::optional<TypeId> expected_element_type_id;
  std::optional<std::int64_t> expected_length;
  if (expected_type_id) // 期望类型必须是array类型
  {
    const auto &expected_type = semantic_ctx.getType(expected_type_id.value());
    auto array_type = dynamic_cast<const ArraySemType *>(&expected_type);
    if (!array_type)
    {
      return std::nullopt;
    }
    expected_element_type_id = array_type->element_type_id_;
    expected_length = array_type->length_;
  }
  if (array_expr->is_repeat_)
  {
    ConstEvaluator const_evaluator;
    auto count_result = const_evaluator.evaluateExpr(
        semantic_ctx, array_expr->repeat_count_.get(), 3,
        func_info.owner_struct_id_);
    if (!count_result ||
        count_result.value().type_id_ !=
            3) // 类型必须是usize,
               // const运算中只有等于期望类型才会返回，因为根本没有引用那些乱七八糟的
    {
      return std::nullopt;
    }
    if (!std::holds_alternative<std::int64_t>(count_result.value().value_))
    {
      return std::nullopt;
    }
    std::int64_t length = std::get<std::int64_t>(count_result.value().value_);
    if (length <= 0 || length > std::numeric_limits<std::uint32_t>::max())
    {
      return std::nullopt;
    }
    if (expected_length && length != expected_length.value())
    {
      return std::nullopt;
    }
    auto element_info = checkExpr(array_expr->repeat_expr_.get(), semantic_ctx,
                                  scope, expected_element_type_id, func_info);
    if (!element_info)
    {
      return std::nullopt;
    }
    // 确定最终类型
    TypeId element_type_id;
    if (expected_element_type_id)
    {
      element_type_id = expected_element_type_id.value();
    }
    else
    {
      element_type_id = element_info.value().type_id_;
    }
    // 检查有无copy
    if (length > 1 && !semantic_ctx.isCopyType(element_type_id))
    {
      return std::nullopt;
    }
    // 拿到返回array类型
    TypeId array_type_id =
        semantic_ctx.getOrCreateArrayTypeId(element_type_id, length);
    return ExprInfo{.type_id_ = array_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = element_info.value().diverges_};
  }
  // 枚举式
  else
  {
    if (array_expr->exprs_.empty() ||
        array_expr->exprs_.size() > std::numeric_limits<std::uint32_t>::max())
    {
      return std::nullopt;
    }
    // 检查长度
    std::int64_t length = array_expr->exprs_.size();
    if (expected_length && length != expected_length.value())
    {
      return std::nullopt;
    }
    std::vector<ExprInfo> element_infos;
    bool diverges = false;
    for (const auto &element_expr : array_expr->exprs_)
    {
      auto element_info = checkExpr(element_expr.get(), semantic_ctx, scope,
                                    expected_element_type_id, func_info);
      if (!element_info)
      {
        return std::nullopt;
      }
      element_infos.push_back(element_info.value());
      // 有一个表达式发散整体就发散
      if (element_info.value().diverges_)
      {
        diverges = true;
      }
    }
    // 确定最终类型
    TypeId element_type_id;
    if (expected_element_type_id)
    {
      element_type_id = expected_element_type_id.value();
    }
    else
    {
      auto lub_type_id = getLubTypeId(element_infos, semantic_ctx);
      if (!lub_type_id)
      {
        return std::nullopt;
      }
      element_type_id = lub_type_id.value();
    }
    TypeId array_type_id =
        semantic_ctx.getOrCreateArrayTypeId(element_type_id, length);
    return ExprInfo{.type_id_ = array_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = diverges};
  }
}

std::optional<ExprInfo> BodyChecker::checkStructExpr(
    const StructExpr *struct_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  const auto &path_segments = struct_expr->path_->path_segments_;
  // 只有一个segment, struct name
  if (path_segments.size() != 1)
  {
    return std::nullopt;
  }
  const auto &path_segment = path_segments[0];
  if (path_segment.generic_args_.size() != 0) // 无泛参
  {
    return std::nullopt;
  }
  StructId struct_id;
  if (path_segment.name_ == "Self")
  {
    if (!func_info.owner_struct_id_)
    {
      return std::nullopt;
    }
    struct_id = func_info.owner_struct_id_.value();
  }
  else
  {
    auto it = semantic_ctx.struct_name_to_id_.find(path_segment.name_);
    if (it == semantic_ctx.struct_name_to_id_.end())
    {
      return std::nullopt;
    }
    struct_id = it->second;
  }
  auto struct_type_id = semantic_ctx.getStructTypeId(struct_id);
  if (!struct_type_id)
  {
    return std::nullopt;
  }
  const auto &struct_info = semantic_ctx.structs_[struct_id];
  // 检查字段名，不重不漏
  std::unordered_set<std::string> seen_fields;
  // struct_expr中的声明type顺序
  std::vector<TypeId> field_type_ids;
  for (const auto &field : struct_expr->struct_fields_)
  {
    std::optional<TypeId> field_type_id;
    for (const auto &member : struct_info.members_)
    {
      if (field.name_ == member.name_)
      {
        field_type_id = member.type_id_;
        break;
      }
    }
    if (!field_type_id)
    {
      return std::nullopt;
    }
    // 有重名
    if (seen_fields.find(field.name_) != seen_fields.end())
    {
      return std::nullopt;
    }
    seen_fields.insert(field.name_);
    field_type_ids.push_back(field_type_id.value());
  }
  if (seen_fields.size() != struct_info.members_.size())
  {
    return std::nullopt;
  }
  // 检查初始化表达式
  bool diverges = false;
  for (std::size_t i = 0; i < struct_expr->struct_fields_.size(); i++)
  {
    auto field_info =
        checkExpr(struct_expr->struct_fields_[i].expr_.get(), semantic_ctx,
                  scope, field_type_ids[i], func_info);
    if (!field_info)
    {
      return std::nullopt;
    }
    // 有一个初始化表达式发撒结构体就发散
    if (field_info.value().diverges_)
    {
      diverges = true;
    }
  }
  if (expected_type_id && !canCoerce(struct_type_id.value(),
                                     expected_type_id.value(), semantic_ctx))
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = struct_type_id.value(),
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ = diverges};
}

std::optional<ExprInfo>
BodyChecker::checkIfExpr(const IfExpr *if_expr, SemanticContext &semantic_ctx,
                         Scope &scope, std::optional<TypeId> expected_type_id,
                         const FuncInfo &func_info)
{
  auto condition_info = checkExpr(if_expr->condition_expr_.get(), semantic_ctx,
                                  scope, 4, func_info); // condition期望为bool
  if (!condition_info)
  {
    return std::nullopt;
  }
  // 没有else
  if (!if_expr->has_else_)
  {
    auto then_info = checkBlockExpr(if_expr->block_expr_.get(), semantic_ctx,
                                    scope, 5, func_info); // block 期望 unit
    if (!then_info)
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(5, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = 5,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ =
                        condition_info.value().diverges_}; // 条件发散即发散
  }
  // 有else
  else
  {
    auto then_info = checkBlockExpr(if_expr->block_expr_.get(), semantic_ctx,
                                    scope, expected_type_id, func_info);
    if (!then_info)
    {
      return std::nullopt;
    }
    std::optional<ExprInfo> else_info;
    // else block
    if (!if_expr->is_else_if_)
    {
      else_info = checkBlockExpr(if_expr->else_block_expr_.get(), semantic_ctx,
                                 scope, expected_type_id, func_info);
    }
    // else if
    else
    {
      else_info = checkIfExpr(if_expr->else_if_expr_.get(), semantic_ctx, scope,
                              expected_type_id, func_info);
    }
    if (!else_info)
    {
      return std::nullopt;
    }
    TypeId result_type_id;
    // 有期望类型，都转换到期望类型
    if (expected_type_id)
    {
      result_type_id = expected_type_id.value();
    }
    // 没有期望类型，LUB
    else
    {
      std::vector<ExprInfo> branch_infos{then_info.value(), else_info.value()};
      auto lub_type_id = getLubTypeId(branch_infos, semantic_ctx);
      if (!lub_type_id)
      {
        return std::nullopt;
      }
      result_type_id = lub_type_id.value();
    }
    // 条件发散或有两个分支且两个分支都发散
    bool diverges =
        condition_info.value().diverges_ ||
        (then_info.value().diverges_ && else_info.value().diverges_);
    return ExprInfo{.type_id_ = result_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = diverges};
  }
}

std::optional<ExprInfo> BodyChecker::checkBreakExpr(
    const BreakExpr *break_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  auto loop_target = getLoopTarget();
  if (!loop_target)
  {
    return std::nullopt;
  }
  std::size_t loop_index = loop_target.value();
  if (break_expr->value_ && loop_stack_[loop_index].kind_ !=
                                LoopKind::Loop) // break若有操作数则一定要是loop
  {
    return std::nullopt;
  }
  auto break_expected_type_id =
      loop_stack_[loop_index].expected_type_id_; // 循环传进来的expected_type
  if (!break_expr->value_) // 没有操作数，提供 unit
  {
    if (break_expected_type_id &&
        !canCoerce(5, break_expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    loop_stack_[loop_index].break_values_.push_back(
        ExprInfo{.type_id_ = 5, .is_place_ = false, .mut_ = false});
  }
  else // 有操作数，循环主体负责检查
  {
    // 先占位，防止checkExpr中插入break_value导致顺序混乱
    std::size_t break_index = loop_stack_[loop_index].break_values_.size();
    loop_stack_[loop_index].break_values_.push_back(ExprInfo{
        .type_id_ = 6, .is_place_ = false, .mut_ = false, .diverges_ = true});
    auto expr_info = checkExpr(break_expr->value_.get(), semantic_ctx, scope,
                               break_expected_type_id,
                               func_info); // 检查是否能转换成loop期望类型
    if (!expr_info)
    {
      return std::nullopt;
    }
    loop_stack_[loop_index].break_values_[break_index] = expr_info.value();
  }
  // break本身类型为never，然后break表达式发散
  return ExprInfo{
      .type_id_ = 6, .is_place_ = false, .mut_ = false, .diverges_ = true};
}

std::optional<ExprInfo> BodyChecker::checkReturnExpr(
    const ReturnExpr *return_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  if (return_expr->value_)
  {
    auto expr_info = checkExpr(return_expr->value_.get(), semantic_ctx, scope,
                               func_info.return_type_id_, func_info);
    if (!expr_info)
    {
      return std::nullopt;
    }
    return ExprInfo{
        .type_id_ = 6, .is_place_ = false, .mut_ = false, .diverges_ = true};
  }
  else
  {
    if (func_info.return_type_id_ != 5)
    {
      return std::nullopt;
    }
    return ExprInfo{
        .type_id_ = 6, .is_place_ = false, .mut_ = false, .diverges_ = true};
  }
}

std::optional<ExprInfo>
BodyChecker::checkContinueExpr(const ContinueExpr *continue_expr,
                               SemanticContext &semantic_ctx, Scope &scope,
                               std::optional<TypeId> expected_type_id,
                               const FuncInfo &func_info)
{
  auto loop_target = getLoopTarget();
  if (!loop_target)
  {
    return std::nullopt;
  }
  return ExprInfo{
      .type_id_ = 6, .is_place_ = false, .mut_ = false, .diverges_ = true};
}

std::optional<ExprInfo> BodyChecker::checkUnaryExpr(
    const UnaryExpr *unary_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  // 期望类型不能穿透
  auto operand_info = checkExpr(unary_expr->expr_.get(), semantic_ctx, scope,
                                std::nullopt, func_info);
  if (!operand_info)
  {
    return std::nullopt;
  }
  if (unary_expr->op_ == UnaryOperator::Negate)
  {
    const auto &operand_type =
        semantic_ctx.getType(operand_info.value().type_id_);
    TypeId result_type_id;
    if (auto reference_type =
            dynamic_cast<const ReferenceSemType *>(&operand_type))
    {
      // 至多一层引用
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      result_type_id = reference_type->inner_type_id_;
    }
    else
    {
      result_type_id = operand_info.value().type_id_;
    }
    if (result_type_id != 0 && result_type_id != 2) // i32 或 isize
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(result_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = result_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = operand_info.value().diverges_};
  }
  else if (unary_expr->op_ == UnaryOperator::Not)
  {
    const auto &operand_type =
        semantic_ctx.getType(operand_info.value().type_id_);
    TypeId result_type_id;
    if (auto reference_type =
            dynamic_cast<const ReferenceSemType *>(&operand_type))
    {
      // 至多只接受一层共享引用
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      result_type_id = reference_type->inner_type_id_;
    }
    else
    {
      result_type_id = operand_info.value().type_id_;
    }
    if (result_type_id != 0 && result_type_id != 1 && result_type_id != 2 &&
        result_type_id != 3 && result_type_id != 4)
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(result_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = result_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = operand_info.value().diverges_};
  }

  else if (unary_expr->op_ == UnaryOperator::Borrow)
  {
    TypeId result_type_id = semantic_ctx.getOrCreateReferenceTypeId(
        operand_info.value().type_id_, false);
    if (expected_type_id &&
        !canCoerce(result_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = result_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = operand_info.value().diverges_};
  }

  else if (unary_expr->op_ == UnaryOperator::BorrowMut)
  {
    // 必须可写（可临时物化）
    if (operand_info.value().is_place_ && !operand_info.value().mut_)
    {
      return std::nullopt;
    }
    TypeId result_type_id = semantic_ctx.getOrCreateReferenceTypeId(
        operand_info.value().type_id_, true);
    if (expected_type_id &&
        !canCoerce(result_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = result_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = operand_info.value().diverges_};
  }
  else if (unary_expr->op_ == UnaryOperator::Dereference)
  {
    auto deref_info =
        dereferenceOnce(operand_info.value().type_id_, semantic_ctx);
    if (!deref_info)
    {
      return std::nullopt;
    }
    // p = &&mut 1i32, **p, (*p)先解引用, &&mut i32 -> &mut i32，再解*(*p)。&mut
    // i32 -> i32, 两个有一个经过共享引用就不可写
    // 本质是不能通过一个共享引用来获得写权限，因为写入者只可能有一个
    bool through_shared = operand_info.value().through_shared_ ||
                          deref_info.value().through_shared_;
    const auto &operand_type =
        semantic_ctx.getType(operand_info.value().type_id_);
    bool mut;
    if (through_shared)
    {
      mut = false;
    }
    // 内外都未经过共享引用，operand只可能是box或 &mut
    else if (dynamic_cast<const ReferenceSemType *>(&operand_type))
    {
      // &mut
      mut = true;
    }
    else if (operand_info.value().is_place_)
    {
      // Box 内容的可写性遵循拥有者
      mut = operand_info.value().mut_;
    }
    else
    {
      // Box 临时物化
      mut = true;
    }
    TypeId result_type_id = deref_info.value().type_id_;
    if (expected_type_id)
    {
      if (!canCoerce(result_type_id, expected_type_id.value(), semantic_ctx))
      {
        return std::nullopt;
      }
      // 最终不可变，但是期望&mut，也不行
      // 因为 q = &mut 1i32, p = &p, *p不能获得一个写入者，写入者只有一个
      if (through_shared)
      {
        const auto &expected_type =
            semantic_ctx.getType(expected_type_id.value());
        auto reference_type =
            dynamic_cast<const ReferenceSemType *>(&expected_type);
        if (reference_type && reference_type->mut_)
        {
          return std::nullopt;
        }
      }
    }
    return ExprInfo{.type_id_ = result_type_id,
                    .is_place_ = true,
                    .mut_ = mut,
                    .diverges_ = operand_info.value().diverges_,
                    .through_shared_ = through_shared};
  }

  return std::nullopt;
}

std::optional<ExprInfo> BodyChecker::checkPathExpr(
    const PathExpr *path_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  if (path_expr->path_segments_.size() > 2 ||
      path_expr->path_segments_.size() == 0)
  {
    return std::nullopt;
  }
  if (path_expr->path_segments_.size() == 1)
  {
    if (path_expr->path_segments_[0].generic_args_.size() != 0)
    {
      return std::nullopt;
    }
    auto var_info = scope.getVar(path_expr->path_segments_[0].name_);
    if (!var_info) // 没找到变量，找全局常量
    {
      auto it = semantic_ctx.func_const_name_to_symbol_.find(
          path_expr->path_segments_[0].name_);
      if (it == semantic_ctx.func_const_name_to_symbol_.end())
      {
        return std::nullopt;
      }
      auto symbol = it->second;
      if (symbol.kind_ == ValueKind::Function)
      {
        return std::nullopt;
      }
      if (semantic_ctx.constants_[symbol.index_].owner_struct_id_)
      {
        return std::nullopt;
      }
      const auto &const_info = semantic_ctx.constants_[symbol.index_];
      if (expected_type_id &&
          !canCoerce(const_info.type_id_.value(), expected_type_id.value(),
                     semantic_ctx))
      {
        return std::nullopt;
      }
      return ExprInfo{.type_id_ = const_info.type_id_.value(),
                      .is_place_ = false,
                      .mut_ = false};
    }
    if (expected_type_id &&
        !canCoerce(var_info->type_id_, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = var_info->type_id_,
                    .is_place_ = true,
                    .mut_ = var_info->mut_};
  }
  else // 2个segment
  {
    if (path_expr->path_segments_[1].generic_args_.size() != 0 ||
        path_expr->path_segments_[0].generic_args_.size() != 0)
    {
      return std::nullopt;
    }
    TypeId type_id_1;
    if (path_expr->path_segments_[0].name_ == "Self")
    {
      if (!func_info.owner_struct_id_)
      {
        return std::nullopt;
      }
      type_id_1 = semantic_ctx.struct_id_to_type_id_
                      .find(func_info.owner_struct_id_.value())
                      ->second;
    }
    else
    {
      auto it = semantic_ctx.struct_name_to_id_.find(
          path_expr->path_segments_[0].name_);
      if (it == semantic_ctx.struct_name_to_id_.end())
      {
        return std::nullopt;
      }
      type_id_1 = semantic_ctx.struct_id_to_type_id_.find(it->second)->second;
    }
    auto name = path_expr->path_segments_[1].name_;
    auto &type = semantic_ctx.getType(type_id_1);
    auto struct_id = dynamic_cast<const StructSemType *>(&type)->struct_id_;
    auto associate_item = semantic_ctx.structs_[struct_id].associate_items_;
    auto symbol_it = associate_item.find(name);
    if (symbol_it == associate_item.end())
    {
      return std::nullopt;
    }
    auto symbol = symbol_it->second;
    if (symbol.kind_ == ValueKind::Constant)
    {
      auto const_id = symbol.index_;
      auto const_info = semantic_ctx.constants_[const_id];
      if (!const_info.owner_struct_id_ ||
          semantic_ctx.struct_id_to_type_id_
                  .find(const_info.owner_struct_id_.value())
                  ->second != type_id_1)
      {
        return std::nullopt;
      }
      if (expected_type_id &&
          !canCoerce(const_info.type_id_.value(), expected_type_id.value(),
                     semantic_ctx))
      {
        return std::nullopt;
      }
      return ExprInfo{.type_id_ = const_info.type_id_.value(),
                      .is_place_ = false,
                      .mut_ = false};
    }
    else
    {
      return std::nullopt;
    }
  }
}

std::optional<ExprInfo> BodyChecker::checkBlockExpr(
    const BlockExpr *block_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  TypeResolver type_resolver;
  TypeId return_type_id;
  scope.enterScope();
  bool diverges = false;
  // 确定尾表达式
  const Expr *tail_expr = block_expr->tail_expr_.get();
  int stmt_count = block_expr->stmts_.size();
  if (!tail_expr && stmt_count > 0)
  {
    auto last_stmt = dynamic_cast<const ExprStmt *>(
        block_expr->stmts_[stmt_count - 1].get());
    if (last_stmt && !last_stmt->has_semicolon_)
    {
      tail_expr = last_stmt->expr_.get();
      stmt_count--;
    }
  }
  for (int i = 0; i < stmt_count; i++)
  {
    const Stmt *stmt = block_expr->stmts_[i].get();
    if (auto let_stmt = dynamic_cast<const LetStmt *>(stmt))
    {
      auto name = let_stmt->ident_;
      std::optional<TypeId> type_id;
      if (!let_stmt->type_)
      {
        type_id = std::nullopt;
      }
      else
      {
        type_id = type_resolver.resolveType(let_stmt->type_.get(), semantic_ctx,
                                            func_info.owner_struct_id_);
        if (!type_id)
        {
          return std::nullopt;
        }
      }
      auto expr_info = checkExpr(let_stmt->expr_.get(), semantic_ctx, scope,
                                 type_id, func_info);
      if (!expr_info)
      {
        return std::nullopt;
      }
      if (expr_info.value().diverges_) // let发散则发散，因为必经
      {
        diverges = true;
      }
      if (type_id)
      {
        scope.bind(VarInfo{.name_ = name,
                           .type_id_ = type_id.value(),
                           .mut_ = let_stmt->mut_});
      }
      else
      {
        scope.bind(VarInfo{.name_ = name,
                           .type_id_ = expr_info.value().type_id_,
                           .mut_ = let_stmt->mut_});
      }
    }
    else // ExprStmt
    {
      auto expr_stmt = dynamic_cast<const ExprStmt *>(stmt);
      const auto *expr = expr_stmt->expr_.get();
      std::optional<ExprInfo> expr_info;
      if (!expr_stmt->has_semicolon_) // 没有分号，需要为 unit
      {
        expr_info = checkExpr(expr, semantic_ctx, scope, 5, func_info);
        if (!expr_info)
        {
          return std::nullopt;
        }
        if (expr_info.value().diverges_)
        {
          diverges = true;
        }
      }
      else
      {
        expr_info =
            checkExpr(expr, semantic_ctx, scope, std::nullopt, func_info);
        if (!expr_info)
        {
          return std::nullopt;
        }
        if (expr_info.value().diverges_)
        {
          diverges = true;
        }
      }
    }
  }
  if (tail_expr)
  {
    auto expr_info =
        checkExpr(tail_expr, semantic_ctx, scope, expected_type_id, func_info);
    if (!expr_info)
    {
      return std::nullopt;
    }
    return_type_id = expr_info.value().type_id_;
    if (expr_info.value().diverges_)
    {
      diverges = true;
    }
  }
  else // 无尾表达式
  {
    if (diverges)
    {
      return_type_id = 6;
    }
    else
    {
      return_type_id = 5;
      if (expected_type_id &&
          !canCoerce(return_type_id, expected_type_id.value(), semantic_ctx))
      {
        return std::nullopt;
      }
    }
  }
  scope.exitScope();
  return ExprInfo{.type_id_ = return_type_id,
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ = diverges};
}

std::optional<ExprInfo> BodyChecker::checkBinaryExpr(
    const BinaryExpr *binary_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  if (binary_expr->op_ == BinaryOperator::Add ||
      binary_expr->op_ == BinaryOperator::Subtract ||
      binary_expr->op_ == BinaryOperator::Multiply ||
      binary_expr->op_ == BinaryOperator::Divide ||
      binary_expr->op_ == BinaryOperator::Remainder)
  {
    // 期望类型不穿透运算符
    auto lhs_info = checkExpr(binary_expr->lhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!lhs_info)
    {
      return std::nullopt;
    }
    auto rhs_info = checkExpr(binary_expr->rhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!rhs_info)
    {
      return std::nullopt;
    }
    const auto &lhs_type = semantic_ctx.getType(lhs_info.value().type_id_);
    TypeId lhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&lhs_type))
    {
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      lhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      lhs_type_id = lhs_info.value().type_id_;
    }
    const auto &rhs_type = semantic_ctx.getType(rhs_info.value().type_id_);
    TypeId rhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&rhs_type))
    {
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      rhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      rhs_type_id = rhs_info.value().type_id_;
    }
    // 算术只支持整数，两侧底层类型必须相同。
    if (lhs_type_id != 0 && lhs_type_id != 1 && lhs_type_id != 2 &&
        lhs_type_id != 3)
    {
      return std::nullopt;
    }
    if (lhs_type_id != rhs_type_id)
    {
      return std::nullopt;
    }

    if (expected_type_id &&
        !canCoerce(lhs_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = lhs_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = lhs_info.value().diverges_ ||
                                 rhs_info.value().diverges_};
  }
  else if (binary_expr->op_ == BinaryOperator::BitAnd ||
           binary_expr->op_ == BinaryOperator::BitOr ||
           binary_expr->op_ == BinaryOperator::BitXor)
  {
    // 期望类型不穿透运算符
    auto lhs_info = checkExpr(binary_expr->lhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!lhs_info)
    {
      return std::nullopt;
    }
    auto rhs_info = checkExpr(binary_expr->rhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!rhs_info)
    {
      return std::nullopt;
    }
    const auto &lhs_type = semantic_ctx.getType(lhs_info.value().type_id_);
    TypeId lhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&lhs_type))
    {
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      lhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      lhs_type_id = lhs_info.value().type_id_;
    }
    const auto &rhs_type = semantic_ctx.getType(rhs_info.value().type_id_);
    TypeId rhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&rhs_type))
    {
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      rhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      rhs_type_id = rhs_info.value().type_id_;
    }
    // 按位运算支持整数和 bool，两侧底层类型必须相同。
    if (lhs_type_id != 0 && lhs_type_id != 1 && lhs_type_id != 2 &&
        lhs_type_id != 3 && lhs_type_id != 4)
    {
      return std::nullopt;
    }
    if (lhs_type_id != rhs_type_id)
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(lhs_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = lhs_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = lhs_info.value().diverges_ ||
                                 rhs_info.value().diverges_};
  }
  else if (binary_expr->op_ == BinaryOperator::ShiftLeft ||
           binary_expr->op_ == BinaryOperator::ShiftRight)
  {
    // 期望类型不穿透运算符，右侧也不从左侧推断类型。
    auto lhs_info = checkExpr(binary_expr->lhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!lhs_info)
    {
      return std::nullopt;
    }
    auto rhs_info = checkExpr(binary_expr->rhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!rhs_info)
    {
      return std::nullopt;
    }
    const auto &lhs_type = semantic_ctx.getType(lhs_info.value().type_id_);
    TypeId lhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&lhs_type))
    {
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      lhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      lhs_type_id = lhs_info.value().type_id_;
    }
    const auto &rhs_type = semantic_ctx.getType(rhs_info.value().type_id_);
    TypeId rhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&rhs_type))
    {
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      rhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      rhs_type_id = rhs_info.value().type_id_;
    }
    // (&)T 与 (&)S， T与S都为整数类型
    if (lhs_type_id != 0 && lhs_type_id != 1 && lhs_type_id != 2 &&
        lhs_type_id != 3)
    {
      return std::nullopt;
    }
    if (rhs_type_id != 0 && rhs_type_id != 1 && rhs_type_id != 2 &&
        rhs_type_id != 3)
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(lhs_type_id, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = lhs_type_id,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = lhs_info.value().diverges_ ||
                                 rhs_info.value().diverges_};
  }
  else if (binary_expr->op_ == BinaryOperator::Less ||
           binary_expr->op_ == BinaryOperator::LessEqual ||
           binary_expr->op_ == BinaryOperator::Greater ||
           binary_expr->op_ == BinaryOperator::GreaterEqual)
  {
    // 操作数独立确定类型，排序结果的期望类型不会传入操作数。
    auto lhs_info = checkExpr(binary_expr->lhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!lhs_info)
    {
      return std::nullopt;
    }
    auto rhs_info = checkExpr(binary_expr->rhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!rhs_info)
    {
      return std::nullopt;
    }
    TypeId lhs_type_id = lhs_info.value().type_id_;
    TypeId rhs_type_id = rhs_info.value().type_id_;
    bool is_outer = true;
    while (true)
    {
      // 每次解引用前先判断是否相等
      if (lhs_type_id == rhs_type_id &&
          (lhs_type_id == 0 || lhs_type_id == 1 || lhs_type_id == 2 ||
           lhs_type_id == 3 || lhs_type_id == 4))
      {
        break;
      }
      const auto &lhs_type = semantic_ctx.getType(lhs_type_id);
      const auto &rhs_type = semantic_ctx.getType(rhs_type_id);
      auto lhs_reference = dynamic_cast<const ReferenceSemType *>(&lhs_type);
      auto rhs_reference = dynamic_cast<const ReferenceSemType *>(&rhs_type);
      if (!lhs_reference || !rhs_reference)
      {
        return std::nullopt;
      }
      // 至少内部要相同
      if (lhs_reference->inner_type_id_ != rhs_reference->inner_type_id_)
      {
        return std::nullopt;
      }
      if (lhs_reference->mut_ != rhs_reference->mut_)
      {
        // 仅最外层允许左侧 &T、右侧 &mut T
        if (is_outer)
        {
          if (!(!lhs_reference->mut_ && rhs_reference->mut_))
          {
            return std::nullopt;
          }
          is_outer = false;
        }
        else
        {
          return std::nullopt;
        }
      }
      lhs_type_id = lhs_reference->inner_type_id_;
      rhs_type_id = rhs_reference->inner_type_id_;
    }
    if (expected_type_id &&
        !canCoerce(4, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{
        .type_id_ = 4,
        .is_place_ = false,
        .mut_ = false,
        .diverges_ =
            lhs_info.value().diverges_ ||
            rhs_info.value().diverges_}; // 引用类型的发散性和最里层的发散性相同
  }
  else if (binary_expr->op_ == BinaryOperator::Equal ||
           binary_expr->op_ == BinaryOperator::NotEqual)
  {
    // 期望类型不会穿透
    auto lhs_info = checkExpr(binary_expr->lhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!lhs_info)
    {
      return std::nullopt;
    }
    auto rhs_info = checkExpr(binary_expr->rhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!rhs_info)
    {
      return std::nullopt;
    }
    // 要求完全相同
    if (lhs_info.value().type_id_ != rhs_info.value().type_id_)
    {
      return std::nullopt;
    }
    if (!semantic_ctx.isPartialEqType(lhs_info.value().type_id_))
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(4, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = 4,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = lhs_info.value().diverges_ ||
                                 rhs_info.value().diverges_};
  }
  else if (binary_expr->op_ == BinaryOperator::LogicalAnd ||
           binary_expr->op_ == BinaryOperator::LogicalOr)
  {
    // 两侧都要做语义检查，期望类型不穿透运算符。
    auto lhs_info = checkExpr(binary_expr->lhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!lhs_info)
    {
      return std::nullopt;
    }
    auto rhs_info = checkExpr(binary_expr->rhs_.get(), semantic_ctx, scope,
                              std::nullopt, func_info);
    if (!rhs_info)
    {
      return std::nullopt;
    }
    // 要求bool是已知期望类型，所以这里支持强制类型转换
    if (!canCoerce(lhs_info.value().type_id_, 4, semantic_ctx) ||
        !canCoerce(rhs_info.value().type_id_, 4, semantic_ctx))
    {
      return std::nullopt;
    }
    if (expected_type_id &&
        !canCoerce(4, expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    // 发散取左侧表达式
    return ExprInfo{.type_id_ = 4,
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = lhs_info.value().diverges_};
  }
  return std::nullopt;
}

std::optional<ExprInfo> BodyChecker::checkAssignExpr(
    const AssignExpr *assign_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  // 检查lhs
  auto lhs_info = checkExpr(assign_expr->lhs_.get(), semantic_ctx, scope,
                            std::nullopt, func_info);
  if (!lhs_info)
  {
    return std::nullopt;
  }
  // 左侧要求可写place
  if (!lhs_info.value().is_place_)
  {
    return std::nullopt;
  }
  if (!lhs_info.value().mut_)
  {
    return std::nullopt;
  }
  std::optional<TypeId> rhs_expected_type_id;
  if (assign_expr->op_ == AssignmentOperator::Assign)
  {
    // 普通赋值以左侧期望右侧
    rhs_expected_type_id = lhs_info.value().type_id_;
  }
  else
  {
    // 复合赋值右侧不期望
    rhs_expected_type_id = std::nullopt;
  }
  auto rhs_info = checkExpr(assign_expr->rhs_.get(), semantic_ctx, scope,
                            rhs_expected_type_id, func_info);
  if (!rhs_info)
  {
    return std::nullopt;
  }
  if (assign_expr->op_ != AssignmentOperator::Assign)
  {
    TypeId lhs_type_id = lhs_info.value().type_id_;
    const auto &rhs_type = semantic_ctx.getType(rhs_info.value().type_id_);
    TypeId rhs_type_id;
    if (auto reference_type = dynamic_cast<const ReferenceSemType *>(&rhs_type))
    {
      // 右侧只允许一层共享引用
      if (reference_type->mut_)
      {
        return std::nullopt;
      }
      rhs_type_id = reference_type->inner_type_id_;
    }
    else
    {
      rhs_type_id = rhs_info.value().type_id_;
    }
    if (assign_expr->op_ == AssignmentOperator::AddAssign ||
        assign_expr->op_ == AssignmentOperator::SubtractAssign ||
        assign_expr->op_ == AssignmentOperator::MultiplyAssign ||
        assign_expr->op_ == AssignmentOperator::DivideAssign ||
        assign_expr->op_ == AssignmentOperator::RemainderAssign)
    {
      // 算术复合赋值：左侧为整数位置，两侧底层类型相同
      if (lhs_type_id != 0 && lhs_type_id != 1 && lhs_type_id != 2 &&
          lhs_type_id != 3)
      {
        return std::nullopt;
      }
      if (lhs_type_id != rhs_type_id)
      {
        return std::nullopt;
      }
    }
    else if (assign_expr->op_ == AssignmentOperator::BitAndAssign ||
             assign_expr->op_ == AssignmentOperator::BitOrAssign ||
             assign_expr->op_ == AssignmentOperator::BitXorAssign)
    {
      // 按位复合赋值：左侧为整数或 bool 位置，两侧底层类型相同
      if (lhs_type_id != 0 && lhs_type_id != 1 && lhs_type_id != 2 &&
          lhs_type_id != 3 && lhs_type_id != 4)
      {
        return std::nullopt;
      }
      if (lhs_type_id != rhs_type_id)
      {
        return std::nullopt;
      }
    }
    else if (assign_expr->op_ == AssignmentOperator::ShiftLeftAssign ||
             assign_expr->op_ == AssignmentOperator::ShiftRightAssign)
    {
      // 移位复合赋值：两侧都是整数，整数类型可以不同
      if (lhs_type_id != 0 && lhs_type_id != 1 && lhs_type_id != 2 &&
          lhs_type_id != 3)
      {
        return std::nullopt;
      }
      if (rhs_type_id != 0 && rhs_type_id != 1 && rhs_type_id != 2 &&
          rhs_type_id != 3)
      {
        return std::nullopt;
      }
    }
    else
    {
      return std::nullopt;
    }
  }
  // 赋值表达式返回类型为 unit
  if (expected_type_id && !canCoerce(5, expected_type_id.value(), semantic_ctx))
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = 5,
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ =
                      lhs_info.value().diverges_ || rhs_info.value().diverges_};
}

bool BodyChecker::canCast(TypeId source_type_id, TypeId target_type_id,
                          SemanticContext &semantic_ctx)
{
  if (target_type_id != 0 && target_type_id != 1 && target_type_id != 2 &&
      target_type_id != 3)
  {
    return false;
  }
  if (source_type_id == 0 || source_type_id == 1 || source_type_id == 2 ||
      source_type_id == 3 || source_type_id == 4)
  {
    return true;
  }
  return false;
}

std::optional<ExprInfo> BodyChecker::checkCastExpr(
    const CastExpr *cast_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  TypeResolver type_resolver;
  auto as_type = type_resolver.resolveType(cast_expr->type_.get(), semantic_ctx,
                                           std::nullopt);
  if (!as_type || as_type.value() != 0 && as_type.value() != 1 &&
                      as_type.value() != 2 && as_type.value() != 3)
  {
    return std::nullopt;
  }
  auto expr_info = checkExpr(cast_expr->expr_.get(), semantic_ctx, scope,
                             std::nullopt, func_info);
  if (!expr_info)
  {
    return std::nullopt;
  }
  if (canCast(expr_info.value().type_id_, as_type.value(), semantic_ctx))
  {
    if (expected_type_id &&
        !canCoerce(as_type.value(), expected_type_id.value(), semantic_ctx))
    {
      return std::nullopt;
    }
    return ExprInfo{.type_id_ = as_type.value(),
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = expr_info.value().diverges_};
  }
  return std::nullopt;
}

std::optional<ExprInfo> BodyChecker::checkCallExpr(
    const CallExpr *call_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  bool diverges = false;
  TypeResolver type_resolver;
  auto callee = call_expr->callee_.get();
  auto path_expr = dynamic_cast<const PathExpr *>(callee);
  FuncInfo source_func_info; // 找到的函数info
  if (!path_expr)
  {
    return std::nullopt;
  }
  if (path_expr->path_segments_.size() > 2 ||
      path_expr->path_segments_.size() == 0)
  {
    return std::nullopt;
  }
  if (path_expr->path_segments_.size() == 1) // 全局函数，内置函数也算
  {
    if (path_expr->path_segments_[0].generic_args_.size())
    {
      return std::nullopt;
    }
    auto name = path_expr->path_segments_[0].name_;
    if (scope.getVar(name)) // 不能与局部变量重名
    {
      return std::nullopt;
    }
    auto type_it = semantic_ctx.func_const_name_to_symbol_.find(name);
    if (type_it == semantic_ctx.func_const_name_to_symbol_.end() ||
        type_it->second.kind_ == ValueKind::Constant)
    {
      return std::nullopt;
    }
    source_func_info = semantic_ctx.functions_[type_it->second.index_];
  }
  else // 2个segment
  {
    if (path_expr->path_segments_[1].generic_args_.size() != 0)
    {
      return std::nullopt;
    }
    const auto &function_name = path_expr->path_segments_[1].name_;
    // 找函数，返回时的参数self已经转化成普通参数了
    if (path_expr->path_segments_[0].name_ == "Box")
    {
      if (path_expr->path_segments_[0].generic_args_.size() != 1) // Box<T>
      {
        return std::nullopt;
      }
      auto type_T_id = type_resolver.resolveType(
          path_expr->path_segments_[0].generic_args_[0].type_.get(),
          semantic_ctx, func_info.owner_struct_id_);
      if (!type_T_id)
      {
        return std::nullopt;
      }
      TypeId box_type_id = semantic_ctx.getOrCreateBoxTypeId(type_T_id.value());
      if (function_name == "new")
      {
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {type_T_id.value()},
                                    .return_type_id_ = box_type_id,
                                    .is_builtin_ = true};
      }
      else if (function_name == "clone")
      {
        if (!semantic_ctx.isCloneType(box_type_id))
        {
          return std::nullopt;
        }
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(box_type_id, false);
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {receiver_type_id},
                                    .return_type_id_ = box_type_id,
                                    .is_builtin_ = true};
      }
      else
      {
        return std::nullopt;
      }
    }
    else if (path_expr->path_segments_[0].name_ == "Vec")
    {
      if (path_expr->path_segments_[0].generic_args_.size() != 1)
      {
        return std::nullopt;
      }
      auto type_T_id = type_resolver.resolveType(
          path_expr->path_segments_[0].generic_args_[0].type_.get(),
          semantic_ctx, func_info.owner_struct_id_);
      if (!type_T_id)
      {
        return std::nullopt;
      }
      TypeId vec_type_id = semantic_ctx.getOrCreateVecTypeId(type_T_id.value());
      if (function_name == "new")
      {
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {},
                                    .return_type_id_ = vec_type_id,
                                    .is_builtin_ = true};
      }
      else if (function_name == "len")
      {
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(vec_type_id, false);
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {receiver_type_id},
                                    .return_type_id_ = 3,
                                    .is_builtin_ = true};
      }
      else if (function_name == "is_empty")
      {
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(vec_type_id, false);
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {receiver_type_id},
                                    .return_type_id_ = 4,
                                    .is_builtin_ = true};
      }
      else if (function_name == "push")
      {
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(vec_type_id, true);
        source_func_info =
            FuncInfo{.name_ = function_name,
                     .args_type_id_ = {receiver_type_id, type_T_id.value()},
                     .return_type_id_ = 5,
                     .is_builtin_ = true};
      }
      else if (function_name == "remove")
      {
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(vec_type_id, true);
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {receiver_type_id, 3},
                                    .return_type_id_ = type_T_id.value(),
                                    .is_builtin_ = true};
      }
      else if (function_name == "clone")
      {
        if (!semantic_ctx.isCloneType(vec_type_id))
        {
          return std::nullopt;
        }
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(vec_type_id, false);
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {receiver_type_id},
                                    .return_type_id_ = vec_type_id,
                                    .is_builtin_ = true};
      }
      else
      {
        return std::nullopt;
      }
    }
    else // Struct、Self 或基本类型（基本类型有clone调用）
    {
      TypeId type_id_1;
      if (path_expr->path_segments_[0].generic_args_.size())
      {
        return std::nullopt;
      }
      if (path_expr->path_segments_[0].name_ == "Self") // Self
      {
        if (!func_info.owner_struct_id_)
        {
          return std::nullopt;
        }
        auto self_type_id =
            semantic_ctx.getStructTypeId(func_info.owner_struct_id_.value());
        if (!self_type_id)
        {
          return std::nullopt;
        }
        type_id_1 = self_type_id.value();
      }
      else
      {
        auto builtin_type_id =
            semantic_ctx.getBuiltinTypeId(path_expr->path_segments_[0].name_);
        if (builtin_type_id)
        {
          type_id_1 = builtin_type_id.value();
        }
        else
        {
          auto struct_type_id =
              semantic_ctx.getStructTypeId(path_expr->path_segments_[0].name_);
          if (!struct_type_id)
          {
            return std::nullopt;
          }
          type_id_1 = struct_type_id.value();
        }
      }
      const auto &type = semantic_ctx.getType(type_id_1);
      std::optional<std::size_t> declared_function_id;
      if (auto struct_type = dynamic_cast<const StructSemType *>(&type))
      {
        const auto &struct_info =
            semantic_ctx.structs_[struct_type->struct_id_];
        auto item_it = struct_info.associate_items_.find(function_name);
        if (item_it != struct_info.associate_items_.end())
        {
          if (item_it->second.kind_ != ValueKind::Function)
          {
            return std::nullopt;
          }
          declared_function_id = item_it->second.index_;
        }
      } 
      // 优先自己定义的函数
      if (declared_function_id)
      {
        source_func_info =
            semantic_ctx.functions_[declared_function_id.value()];
      }
      // 无定义说明为clone
      else
      {
        if (function_name != "clone" || !semantic_ctx.isCloneType(type_id_1))
        {
          return std::nullopt;
        }
        // &T
        TypeId receiver_type_id =
            semantic_ctx.getOrCreateReferenceTypeId(type_id_1, false);
        source_func_info = FuncInfo{.name_ = function_name,
                                    .args_type_id_ = {receiver_type_id},
                                    .return_type_id_ = type_id_1,
                                    .is_builtin_ = true};
      }
    }
  }
  // 检查参数是否符合类型
  // 解决self(struct impl可能有，vec,box已经解决)
  auto param_type_ids = source_func_info.args_type_id_;
  // 将self加入第一个检查参数，方便后续统一检查
  if (source_func_info.self_param_)
  {
    if (!source_func_info.owner_struct_id_)
    {
      return std::nullopt;
    }
    auto self_type_id =
        semantic_ctx.getStructTypeId(source_func_info.owner_struct_id_.value());
    if (!self_type_id)
    {
      return std::nullopt;
    }
    // 生成需要的S或&S或&mut S
    TypeId receiver_type_id = self_type_id.value();
    const auto &self_param = source_func_info.self_param_.value();
    if (self_param.is_reference_)
    {
      receiver_type_id = semantic_ctx.getOrCreateReferenceTypeId(
          receiver_type_id, self_param.mut_);
    }
    param_type_ids.insert(param_type_ids.begin(), receiver_type_id);
  }
  if (param_type_ids.size() != call_expr->arguments_.size())
  {
    return std::nullopt;
  }
  for (int i = 0; i < param_type_ids.size(); i++)
  {
    auto expr_info = checkExpr(call_expr->arguments_[i].get(), semantic_ctx,
                               scope, param_type_ids[i], func_info);
    if (!expr_info)
    {
      return std::nullopt;
    }
    if (expr_info.value().diverges_)
    {
      diverges = true;
    }
  }
  if (expected_type_id && !canCoerce(source_func_info.return_type_id_,
                                     expected_type_id.value(), semantic_ctx))
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = source_func_info.return_type_id_,
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ = diverges};
}

std::optional<ExprInfo> BodyChecker::checkIndexExpr(
    const IndexExpr *index_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  // 检查 base
  auto base_info = checkExpr(index_expr->base_.get(), semantic_ctx, scope,
                             std::nullopt, func_info);
  if (!base_info)
  {
    return std::nullopt;
  }
  TypeId type_id = base_info.value().type_id_;
  bool mut;
  if (base_info.value().is_place_)
  {
    mut = base_info.value().mut_;
  }
  else
  {
    // 值表达式临时物化
    mut = true;
  }
  bool through_shared = base_info.value().through_shared_;
  if (through_shared)
  {
    mut = false;
  }
  // 自动解引用，直到遇到第一个数组或 Vec。
  while (true)
  {
    const auto &type = semantic_ctx.getType(type_id);
    if (dynamic_cast<const ArraySemType *>(&type) ||
        dynamic_cast<const VecSemType *>(&type))
    {
      break;
    }
    auto deref_info = dereferenceOnce(type_id, semantic_ctx);
    if (!deref_info)
    {
      return std::nullopt;
    }
    if (deref_info.value().through_shared_)
    {
      through_shared = true;
    }
    if (through_shared)
    {
      mut = false;
    }
    else if (dynamic_cast<const ReferenceSemType *>(&type))
    {
      // &mut, 可以提供mut
      mut = true;
    }
    // 解引用box可写性不变
    type_id = deref_info.value().type_id_;
  }
  // index期望类型 usize
  auto index_info =
      checkExpr(index_expr->index_.get(), semantic_ctx, scope, 3, func_info);
  if (!index_info)
  {
    return std::nullopt;
  }
  // 只有never和usize满足，但是never需要拒绝
  if (index_info.value().type_id_ != 3)
  {
    return std::nullopt;
  }
  const auto &type = semantic_ctx.getType(type_id);
  TypeId element_type_id;
  if (auto array_type = dynamic_cast<const ArraySemType *>(&type))
  {
    element_type_id = array_type->element_type_id_;
  }
  else
  {
    // Vec
    auto vec_type = dynamic_cast<const VecSemType *>(&type);
    element_type_id = vec_type->element_type_id_;
    if (!mut)
    {
      // 不可变的vec里面存的都是写入者的共享引用，相当于一层共享引用
      through_shared = true;
    }
  }
  if (expected_type_id &&
      !canCoerce(element_type_id, expected_type_id.value(), semantic_ctx))
  {
    return std::nullopt;
  }
  if (expected_type_id && through_shared)
  {
    // 如果期望&mut还经过了共享引用，那确实不能满足
    const auto &expected_type = semantic_ctx.getType(expected_type_id.value());
    auto reference_type =
        dynamic_cast<const ReferenceSemType *>(&expected_type);
    if (reference_type && reference_type->mut_)
    {
      return std::nullopt;
    }
  }
  return ExprInfo{.type_id_ = element_type_id,
                  .is_place_ = true,
                  .mut_ = mut,
                  .diverges_ = base_info.value().diverges_ ||
                               index_info.value().diverges_,
                  .through_shared_ = through_shared};
}

std::optional<ExprInfo> BodyChecker::checkMemberExpr(
    const MemberExpr *member_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  // 检查base，也查其中有没有经过共享引用
  auto expr_info = checkExpr(member_expr->base_.get(), semantic_ctx, scope,
                             std::nullopt, func_info);
  if (!expr_info)
  {
    return std::nullopt;
  }
  TypeId type_id = expr_info.value().type_id_;
  bool mut = expr_info.value().mut_; // 初始是否mut
  if (!expr_info.value().is_place_)  // 临时物化
  {
    mut = true;
  }
  bool through_shared =
      expr_info.value()
          .through_shared_; // 解决a.b.c保留a.b是否经过共享引用的信息
  if (through_shared)
  {
    mut = false;
  }
  // 自动解引用，直到找到结构体
  while (true)
  {
    const auto &type = semantic_ctx.getType(type_id);
    if (dynamic_cast<const StructSemType *>(&type))
    {
      break;
    }
    auto dere_info = dereferenceOnce(type_id, semantic_ctx);
    if (!dere_info)
    {
      return std::nullopt;
    }
    if (dere_info.value().through_shared_)
    {
      through_shared = true;
    }
    if (through_shared)
    {
      mut = false;
    }
    else if (dynamic_cast<const ReferenceSemType *>(&type))
    {
      mut = true;
    }
    type_id = dere_info.value().type_id_;
  }
  // 查找字段
  const auto &type = semantic_ctx.getType(type_id);
  auto struct_id = dynamic_cast<const StructSemType *>(&type)->struct_id_;
  const auto &struct_info = semantic_ctx.structs_[struct_id];
  std::optional<TypeId> member_type_id;
  for (const auto &field_info : struct_info.members_)
  {
    if (field_info.name_ == member_expr->member_)
    {
      member_type_id = field_info.type_id_;
      break;
    }
  }
  if (!member_type_id)
  {
    return std::nullopt;
  }
  if (expected_type_id && !canCoerce(member_type_id.value(),
                                     expected_type_id.value(), semantic_ctx))
  {
    return std::nullopt;
  }
  if (expected_type_id &&
      through_shared) // 如果经过共享引用，那么就无法满足&mut的要求了
  {
    const auto &expected_type = semantic_ctx.getType(expected_type_id.value());
    auto reference_type =
        dynamic_cast<const ReferenceSemType *>(&expected_type);
    if (reference_type && reference_type->mut_)
    {
      return std::nullopt;
    }
  }
  return ExprInfo{.type_id_ = member_type_id.value(),
                  .is_place_ = true,
                  .mut_ = mut,
                  .diverges_ = expr_info.value().diverges_,
                  .through_shared_ = through_shared};
}

std::optional<ExprInfo>
BodyChecker::checkMethodCallExpr(const MethodCallExpr *method_call_expr,
                                 SemanticContext &semantic_ctx, Scope &scope,
                                 std::optional<TypeId> expected_type_id,
                                 const FuncInfo &func_info)
{
  // 方法段不允许类型泛型参数
  if (!method_call_expr->method_.generic_args_.empty())
  {
    return std::nullopt;
  }
  auto base_info = checkExpr(method_call_expr->base_.get(), semantic_ctx, scope,
                             std::nullopt, func_info);
  if (!base_info)
  {
    return std::nullopt;
  }
  TypeId type_id = base_info.value().type_id_;
  bool mut;
  if (base_info.value().is_place_)
  {
    mut = base_info.value().mut_;
  }
  else
  {
    // 临时物化
    mut = true;
  }
  bool through_shared = base_info.value().through_shared_;
  if (through_shared)
  {
    mut = false;
  }
  std::optional<MethodInfo> selected_method;
  ReceiverAdjustmentKind receiver_adjustment;
  while (true)
  {
    auto direct_method =
        findMethod(type_id, method_call_expr->method_.name_, semantic_ctx);
    if (direct_method)
    {
      selected_method = direct_method.value();
      receiver_adjustment = ReceiverAdjustmentKind::Direct;
      break;
    }
    TypeId shared_type_id =
        semantic_ctx.getOrCreateReferenceTypeId(type_id, false);
    auto shared_method = findMethod(
        shared_type_id, method_call_expr->method_.name_, semantic_ctx);
    if (shared_method)
    {
      selected_method = shared_method.value();
      receiver_adjustment = ReceiverAdjustmentKind::Borrow;
      break;
    }
    TypeId mutable_type_id =
        semantic_ctx.getOrCreateReferenceTypeId(type_id, true);
    auto mutable_method = findMethod(
        mutable_type_id, method_call_expr->method_.name_, semantic_ctx);
    if (mutable_method)
    {
      selected_method = mutable_method.value();
      receiver_adjustment = ReceiverAdjustmentKind::BorrowMut;
      break;
    }
    // 当前层没有匹配，解引用
    const auto &type = semantic_ctx.getType(type_id);
    auto deref_info = dereferenceOnce(type_id, semantic_ctx);
    if (!deref_info)
    {
      return std::nullopt;
    }
    if (deref_info.value().through_shared_)
    {
      through_shared = true;
    }
    if (through_shared)
    {
      mut = false;
    }
    // &mut可以恢复mut
    else if (dynamic_cast<const ReferenceSemType *>(&type))
    {
      mut = true;
    }
    // Box 继承当前权限
    type_id = deref_info.value().type_id_;
  }
  if (receiver_adjustment == ReceiverAdjustmentKind::BorrowMut)
  {
    // 新建 &mut T，需要T可变
    if (!mut)
    {
      return std::nullopt;
    }
  }
  else if (receiver_adjustment == ReceiverAdjustmentKind::Direct)
  {
    const auto &receiver_type =
        semantic_ctx.getType(selected_method.value().receiver_type_id_);
    auto reference_type =
        dynamic_cast<const ReferenceSemType *>(&receiver_type);
    // 若是&mut直接匹配上，那么就要求不经过共享引用取得
    // &mut T直接传入
    if (reference_type && reference_type->mut_ && through_shared)
    {
      return std::nullopt;
    }
  }
  const auto &method_info = selected_method.value();
  // 检查普通参数
  if (method_info.args_type_id_.size() != method_call_expr->arguments_.size())
  {
    return std::nullopt;
  }
  bool diverges = base_info.value().diverges_;
  for (std::size_t i = 0; i < method_info.args_type_id_.size(); i++)
  {
    // 声明参数类型为期望类型
    auto arg_info =
        checkExpr(method_call_expr->arguments_[i].get(), semantic_ctx, scope,
                  method_info.args_type_id_[i], func_info);
    if (!arg_info)
    {
      return std::nullopt;
    }
    if (arg_info.value().diverges_)
    {
      diverges = true;
    }
  }
  if (expected_type_id && !canCoerce(method_info.return_type_id_,
                                     expected_type_id.value(), semantic_ctx))
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = method_info.return_type_id_,
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ = diverges,
                  .through_shared_ = false};
}

std::optional<ExprInfo> BodyChecker::checkLoopExpr(
    const LoopExpr *loop_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  loop_stack_.push_back(LoopInfo{.kind_ = LoopKind::Loop,
                                 .expected_type_id_ = expected_type_id,
                                 .break_values_ = {}});
  auto body_info = checkBlockExpr(loop_expr->block_expr_.get(), semantic_ctx,
                                  scope, 5, func_info); // block期望类型为()
  if (!body_info)
  {
    return std::nullopt;
  }
  auto loop_info = std::move(loop_stack_.back());
  loop_stack_.pop_back();
  // 若无break, loop发散
  if (loop_info.break_values_.empty())
  {
    return ExprInfo{
        .type_id_ = 6, .is_place_ = false, .mut_ = false, .diverges_ = true};
  }
  // 根据所有 break 操作数判断发散状态
  bool diverges = true;
  for (const auto &break_value_info : loop_info.break_values_)
  {
    if (!break_value_info.diverges_)
    {
      diverges = false;
      break;
    }
  }
  if (expected_type_id)
  {
    return ExprInfo{.type_id_ = expected_type_id.value(),
                    .is_place_ = false,
                    .mut_ = false,
                    .diverges_ = diverges};
  }
  auto result_type_id = getLubTypeId(loop_info.break_values_, semantic_ctx);
  if (!result_type_id)
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = result_type_id.value(),
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ = diverges};
}

std::optional<ExprInfo> BodyChecker::checkWhileExpr(
    const WhileExpr *while_expr, SemanticContext &semantic_ctx, Scope &scope,
    std::optional<TypeId> expected_type_id, const FuncInfo &func_info)
{
  loop_stack_.push_back(LoopInfo{.kind_ = LoopKind::While,
                                 .expected_type_id_ = 5,
                                 .break_values_ = {}}); // block期望类型为()
  // 条件中不能continue, break
  loop_stack_.push_back(LoopInfo{.kind_ = LoopKind::WhileCondition,
                                 .expected_type_id_ = std::nullopt,
                                 .break_values_ = {}});
  auto condition_info =
      checkExpr(while_expr->condition_expr_.get(), semantic_ctx, scope, 4,
                func_info); // condition期望类型为 bool
  if (!condition_info)
  {
    return std::nullopt;
  }
  loop_stack_.pop_back();
  auto body_info = checkBlockExpr(while_expr->block_expr_.get(), semantic_ctx,
                                  scope, 5, func_info);
  if (!body_info)
  {
    return std::nullopt;
  }
  loop_stack_.pop_back();
  if (expected_type_id &&
      !canCoerce(5, expected_type_id.value(), semantic_ctx)) // while类型为()
  {
    return std::nullopt;
  }
  return ExprInfo{.type_id_ = 5,
                  .is_place_ = false,
                  .mut_ = false,
                  .diverges_ =
                      condition_info.value().diverges_}; // 和条件同发散
}
