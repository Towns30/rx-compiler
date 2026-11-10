#pragma once

#include "../semantic/SemanticContext.h"
#include "IR.h"
#include "Type.h"

namespace ir
{

class IRGenerator
{
public:
  Module generateIR(const Crate* crate, SemanticContext semantic_ctx);
private:

};

} // namespace ir