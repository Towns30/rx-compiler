#include "SemanticAnalyzer.h"

bool SemanticAnalyzer::SemanticCheck(const Crate &crate, SemanticContext& semantic_ctx)
{
  DeclarationAnalyzer decl_collector;
  semantic_ctx.initBuiltinType();
  if (!decl_collector.DeclCollect(crate, semantic_ctx))
  {
    return false;
  }
  semantic_ctx.initStructType();
  
  return true;
}