#pragma once
#include "../ast/Ast.h"
#include "DeclarationAnalyzer.h"
#include "TypeResolver.h"
#include "ConstEvaluator.h"

namespace semantic
{

class SemanticAnalyzer
{
public:
  bool SemanticCheck(const Crate &crate, SemanticContext& semantic_ctx);
};

} // namespace semantic
