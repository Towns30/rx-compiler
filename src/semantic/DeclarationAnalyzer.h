#pragma once
#include "../ast/Ast.h"
#include "SemanticContext.h"

namespace semantic
{

class DeclarationAnalyzer
{
public:
  bool DeclCollect(const Crate &crate, SemanticContext &semanitc_ctx);
};

} // namespace semantic
