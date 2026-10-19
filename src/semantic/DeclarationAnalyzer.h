#pragma once
#include "../ast/Ast.h"
#include "SemanticContext.h"

class DeclarationAnalyzer
{
public:
  bool DeclCollect(const Crate &crate, SemanticContext &semanitc_ctx);
};