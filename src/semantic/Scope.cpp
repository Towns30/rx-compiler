#include "Scope.h"
#include <utility>

void Scope::enterScope()
{
  scopes_.push_back(std::unordered_map<std::string, VarInfo>());
}

void Scope::exitScope() { scopes_.pop_back(); }

void Scope::bind(VarInfo var)
{
  scopes_.back().insert_or_assign(var.name_, var);
}

const VarInfo *Scope::getVar(const std::string &name)
{
  for (int i = scopes_.size(); i >= 1; i--)
  {
    auto &scope = scopes_[i - 1];
    auto it = scope.find(name);
    if (it != scope.end())
    {
      return &it->second;
    }
  }
  return nullptr;
}
