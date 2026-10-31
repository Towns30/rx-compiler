#include "SemanticAnalyzer.h"
#include "BodyChecker.h"

bool SemanticAnalyzer::SemanticCheck(const Crate &crate, SemanticContext& semantic_ctx)
{
  DeclarationAnalyzer decl_collector;
  TypeResolver type_resolver;
  ConstEvaluator const_evaluator;
  BodyChecker body_checker;
  semantic_ctx.initBuiltinType();
  semantic_ctx.initBuiltinFunctions();
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
  if (!semantic_ctx.checkMain())
  {
    return false;
  }
  if (!type_resolver.checkStructCycle(semantic_ctx))
  {
    return false;
  }
  if (!semantic_ctx.checkDerives())
  {
    return false;
  }
  if (!body_checker.checkAll(&crate, semantic_ctx))
  {
    return false;
  }
  return true;
}
