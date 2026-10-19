#include "SemanticAnalyzer.h"

bool SemanticAnalyzer::SemanticCheck(const Crate &crate, SemanticContext& semantic_ctx)
{
  DeclarationAnalyzer decl_collector;
  if (!decl_collector.DeclCollect(crate, semantic_ctx))
  {
    return false;
  }
  
  return true;
}