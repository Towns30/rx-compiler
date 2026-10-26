#pragma once

#include "Symbols.h"
#include "Types.h"

class Scope
{
public:
  void enterScope();
  void exitScope();
  void bind(VarInfo var);
  const VarInfo *getVar(const std::string &name);

private:
  std::vector<std::unordered_map<std::string, VarInfo>> scopes_;
};