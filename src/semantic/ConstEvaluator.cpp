#include "ConstEvaluator.h"

bool ConstEvaluator::evaluateAll(SemanticContext &ctx)
{
  for (int i = 0; i < ctx.constants_.size(); i++)
  {
    ConstInfo const_info = ctx.constants_[i];
    if (const_info.status_ == ConstEvaluateStatus::Done)
    {
      continue;
    }
    std::optional<StructId> owner_struct_id = const_info.owner_struct_id_;
    if (!evaluateConst(ctx, i, owner_struct_id))
    {
      return false;
    }
  }
  return true;
}

std::optional<ConstResult>
ConstEvaluator::evaluateExpr(SemanticContext &ctx, const Expr *expr,
                             std::optional<TypeId> expected_type,
                             std::optional<StructId> owner_struct_id)
{
  if (auto literal_expr = dynamic_cast<const LiteralExpr *>(expr))
  {
    return evaluateLiteralExpr(ctx, literal_expr, expected_type);
  }
  else if (auto path_expr = dynamic_cast<const PathExpr *>(expr))
  {
    return evaluatePathExpr(ctx, path_expr, owner_struct_id, expected_type);
  }
  else if (auto unary_expr = dynamic_cast<const UnaryExpr *>(expr))
  {
    return evaluateUnaryExpr(ctx, unary_expr, owner_struct_id, expected_type);
  }
  else
  {
    return std::nullopt;
  }
}

bool ConstEvaluator::evaluateConst(SemanticContext &ctx, int const_id,
                                   std::optional<StructId> owner_struct_id)
{
  ConstInfo &const_info = ctx.constants_[const_id];
  if (!const_info.type_id_)
  {
    return false;
  }
  if (const_info.status_ == ConstEvaluateStatus::Evaluating)
  {
    return false;
  }
  const_info.status_ = ConstEvaluateStatus::Evaluating;
  auto expr = const_info.declaration_->const_value_.get();
  auto result =
      evaluateExpr(ctx, expr, const_info.type_id_, const_info.owner_struct_id_);
  if (result == std::nullopt)
  {
    return false;
  }
  const_info.type_id_ = result.value().type_id_;
  const_info.value_ = result.value().value_;
  const_info.status_ = ConstEvaluateStatus::Done;
  return true;
}

std::optional<ConstResult>
ConstEvaluator::evaluateLiteralExpr(SemanticContext &ctx,
                                    const LiteralExpr *literal_expr,
                                    std::optional<TypeId> expected_type)
{
  if (literal_expr->is_int_)
  {
    auto type_id = getIntegerTypeId(literal_expr->int_type_);
    if (type_id == std::nullopt)
    {
      if (expected_type != std::nullopt)
      {
        if (expected_type.value() != 0 && expected_type.value() != 1 &&
            expected_type.value() != 2 && expected_type.value() != 3)
        {
          return std::nullopt;
        }
        type_id = expected_type.value();
      }
      else
      {
        type_id = 0; // 无期望也无推断，为i32
      }
    }
    else
    {
      if (expected_type != std::nullopt &&
          expected_type.value() != type_id.value())
      {
        return std::nullopt;
      }
    }
    return ConstResult{.type_id_ = type_id.value(),
                       .value_ =
                           static_cast<std::int64_t>(literal_expr->int_value_)};
  }
  else
  {
    if (expected_type != std::nullopt && expected_type.value() != 4)
    {
      return std::nullopt;
    }
    return ConstResult{.type_id_ = 4, .value_ = literal_expr->bool_value_};
  }
}

std::optional<ConstResult>
ConstEvaluator::evaluatePathExpr(SemanticContext &ctx,
                                 const PathExpr *path_expr,
                                 std::optional<StructId> owner_struct_id,
                                 std::optional<TypeId> expected_type)
{
  if (path_expr->path_segments_.size() > 2 ||
      path_expr->path_segments_.size() == 0)
  {
    return std::nullopt;
  }
  if (path_expr->path_segments_.size() == 1)
  {
    auto const_name = path_expr->path_segments_[0].name_;
    if (path_expr->path_segments_[0].generic_args_.size() != 0)
    {
      return std::nullopt;
    }
    if (const_name == "Self")
    {
      return std::nullopt; // 单个Self不行
    }
    auto it = ctx.func_const_name_to_symbol_.find(const_name);
    if (it == ctx.func_const_name_to_symbol_.end())
    {
      return std::nullopt;
    }
    if (it->second.kind_ == ValueKind::Function)
    {
      return std::nullopt;
    }
    int index = it->second.index_;
    auto const_info = ctx.constants_[index];
    if (const_info.status_ == ConstEvaluateStatus::Done)
    {
      if (expected_type != std::nullopt &&
          expected_type.value() != const_info.type_id_)
      {
        return std::nullopt;
      }
      return ConstResult{.type_id_ = const_info.type_id_.value(),
                         .value_ = const_info.value_.value()};
    }
    else if (const_info.status_ ==
             ConstEvaluateStatus::Evaluating) // 没算完，则有环，废了
    {
      return std::nullopt;
    }
    else
    {
      if (!evaluateConst(ctx, index, owner_struct_id))
      {
        return std::nullopt;
      }
      if (expected_type != std::nullopt &&
          expected_type.value() != const_info.type_id_)
      {
        return std::nullopt;
      }
      return ConstResult{.type_id_ = ctx.constants_[index].type_id_.value(),
                         .value_ = ctx.constants_[index].value_.value()};
    }
  }
  if (path_expr->path_segments_.size() == 2)
  {
    auto name_1 = path_expr->path_segments_[0].name_;
    auto name_2 = path_expr->path_segments_[1].name_;
    if (path_expr->path_segments_[0].generic_args_.size() != 0 ||
        (path_expr->path_segments_[1].generic_args_.size() != 0))
    {
      return std::nullopt;
    }
    if (name_1 == "Self")
    {
      if (owner_struct_id == std::nullopt)
      {
        return std::nullopt;
      }
      name_1 = ctx.structs_[owner_struct_id.value()].name_;
    }
    auto struct_it = ctx.struct_name_to_id_.find(name_1);
    if (struct_it == ctx.struct_name_to_id_.end())
    {
      return std::nullopt;
    }
    auto const_it =
        ctx.structs_[struct_it->second].associate_items_.find(name_2);
    if (const_it == ctx.structs_[struct_it->second].associate_items_.end() ||
        const_it->second.kind_ == ValueKind::Function)
    {
      return std::nullopt;
    }
    auto const_info = ctx.constants_[const_it->second.index_];
    if (const_info.owner_struct_id_ == std::nullopt ||
        const_info.owner_struct_id_.value() != struct_it->second)
    {
      return std::nullopt;
    }
    int index = const_it->second.index_;
    if (const_info.status_ == ConstEvaluateStatus::Done)
    {
      if (expected_type != std::nullopt &&
          expected_type.value() != const_info.type_id_)
      {
        return std::nullopt;
      }
      return ConstResult{.type_id_ = const_info.type_id_.value(),
                         .value_ = const_info.value_.value()};
    }
    else if (const_info.status_ ==
             ConstEvaluateStatus::Evaluating) // 没算完，则有环，废了
    {
      return std::nullopt;
    }
    else
    {
      if (!evaluateConst(ctx, index, owner_struct_id))
      {
        return std::nullopt;
      }
      if (expected_type != std::nullopt &&
          expected_type.value() != const_info.type_id_)
      {
        return std::nullopt;
      }
      return ConstResult{.type_id_ = ctx.constants_[index].type_id_.value(),
                         .value_ = ctx.constants_[index].value_.value()};
    }
  }
  return std::nullopt;
}

std::optional<ConstResult>
ConstEvaluator::evaluateUnaryExpr(SemanticContext &ctx,
                                  const UnaryExpr *unary_expr,
                                  std::optional<StructId> owner_struct_id,
                                  std::optional<TypeId> expected_type)
{
  if (unary_expr->op_ != UnaryOperator::Negate)
  {
    return std::nullopt;
  }
  if (auto path_expr = dynamic_cast<const PathExpr *>(unary_expr->expr_.get()))
  {
    auto result =
        evaluatePathExpr(ctx, path_expr, owner_struct_id, std::nullopt);
    if (result == std::nullopt)
    {
      return std::nullopt;
    }
    if (expected_type != std::nullopt &&
        result->type_id_ != expected_type.value())
    {
      return std::nullopt;
    }
    if (result.value().type_id_ == 1 || result.value().type_id_ == 3 ||
        result.value().type_id_ == 4)
    {
      return std::nullopt;
    }
    auto *value = std::get_if<std::int64_t>(&result->value_);
    if (!value)
    {
      return std::nullopt;
    }
    result->value_ = -*value;
    return result;
  }
  else if (auto literal_expr =
               dynamic_cast<const LiteralExpr *>(unary_expr->expr_.get()))
  {
    auto result = evaluateLiteralExpr(ctx, literal_expr, std::nullopt);
    if (result == std::nullopt)
    {
      return std::nullopt;
    }
    if (expected_type != std::nullopt &&
        result->type_id_ != expected_type.value())
    {
      return std::nullopt;
    }
    if (result.value().type_id_ == 1 || result.value().type_id_ == 3 ||
        result.value().type_id_ == 4)
    {
      return std::nullopt;
    }
    auto *value = std::get_if<std::int64_t>(&result->value_);
    if (!value)
    {
      return std::nullopt;
    }
    result->value_ = -*value;
    return result;
  }
  else
  {
    return std::nullopt;
  }
}

std::optional<TypeId> ConstEvaluator::getIntegerTypeId(IntegerType integer_type)
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
