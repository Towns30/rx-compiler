#pragma once
#include "../ast/Ast.h"
#include "DeclarationAnalyzer.h"

class SemanticAnalyzer
{
public:
  bool SemanticCheck(const Crate &crate, SemanticContext& semantic_ctx);
};