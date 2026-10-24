#include "SemanticAnalyzer.h"

bool SemanticAnalyzer::SemanticCheck(const Crate &crate, SemanticContext& semantic_ctx)
{
  DeclarationAnalyzer decl_collector;
  TypeResolver type_resolver;
  ConstEvaluator const_evaluator;
  semantic_ctx.initBuiltinType();
  if (!decl_collector.DeclCollect(crate, semantic_ctx))
  {
    return false;
  }
  semantic_ctx.initStructType();
  if (!type_resolver.resolveConstTypes(semantic_ctx))
  {
    return false;
  }
  if (!const_evaluator.evaluateAll(semantic_ctx))
  {
    return false;
  }
  if (!type_resolver.resolveFuncStructTypes(semantic_ctx))
  {
    return false;
  }
  return true;
}