#pragma once

#include "ConstEvaluator.h"
#include "SemanticContext.h"
#include "Symbols.h"
#include "Types.h"

class TypeResolver
{
public:
  std::optional<TypeId> resolveType(const Type *type,
                                    SemanticContext &semantic_ctx,
                                    std::optional<StructId> owner_struct_id);
  bool resolveConstTypes(SemanticContext &semantic_ctx);
  bool resolveFuncStructTypes(SemanticContext &semantic_ctx);
};