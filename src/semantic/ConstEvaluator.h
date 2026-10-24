#pragma once

#include "SemanticContext.h"
#include <optional>

struct ConstResult
{
  TypeId type_id_;
  ConstValue value_;
};

class ConstEvaluator
{
public:
  bool evaluateAll(SemanticContext &ctx);
  std::optional<ConstResult>
  evaluateExpr(SemanticContext &ctx, const Expr *expr,
               std::optional<TypeId> expected_type,
               std::optional<StructId> owner_struct_id);

  bool evaluateConst(SemanticContext &ctx, int const_id,
                     std::optional<StructId> owner_struct_id);
  std::optional<ConstResult>
  evaluateLiteralExpr(SemanticContext &ctx, const LiteralExpr *literal_expr, std::optional<TypeId> expected_type);
  std::optional<ConstResult>
  evaluatePathExpr(SemanticContext &ctx, const PathExpr *path_expr,
                   std::optional<StructId> owner_struct_id, std::optional<TypeId> expected_type);
  std::optional<ConstResult>
  evaluateUnaryExpr(SemanticContext &ctx, const UnaryExpr *unary_expr,
                    std::optional<StructId> owner_struct_id, std::optional<TypeId> expected_type);
  std::optional<TypeId> getIntegerTypeId(IntegerType integer_type);
};