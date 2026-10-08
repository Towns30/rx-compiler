#include "Ast.h"

GenericArg::GenericArg(bool is_type, std::unique_ptr<Type> type,
                       std::optional<std::string> life_time)
    : is_type_(is_type), type_(std::move(type)),
      life_time_(std::move(life_time))
{
}

void GenericArg::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "GenericArg " << (is_type_ ? "type" : "lifetime") << '\n';
  if (is_type_)
  {
    type_->print(space_num + 1);
  }
  else
  {
    PrintSpace(space_num + 1);
    std::cout << "Lifetime ";
    if (life_time_)
    {
      std::cout << *life_time_;
    }
    else
    {
      std::cout << "(none)";
    }
    std::cout << '\n';
  }
}

void PathSegment::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "PathSegment " << name_ << '\n';
  for (auto &arg : generic_args_)
  {
    arg.print(space_num + 1);
  }
}

void Path::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "Path" << (absolute_ ? " absolute" : "") << '\n';
  for (auto &segment : segments_)
  {
    segment.print(space_num + 1);
  }
}

void UseItem::print(int space_num) {}

void ConstItem::print(int space_num) {}

void ImplItem::print(int space_num) {}

void StructItem::print(int space_num) {}

void ExprStmt::print(int space_num) {}

void UnitExpr::print(int space_num) {}

void PathExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "PathExpr\n";
  for (auto &segment : path_segments_)
  {
    segment.print(space_num + 1);
  }
}

void ArrayExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ArrayExpr" << (is_repeat_ ? " repeat" : "") << '\n';
  if (is_repeat_)
  {
    PrintSpace(space_num + 1);
    std::cout << "Element\n";
    repeat_expr_->print(space_num + 2);
    PrintSpace(space_num + 1);
    std::cout << "Count\n";
    repeat_count_->print(space_num + 2);
  }
  else
  {
    for (auto &expr : exprs_)
    {
      expr->print(space_num + 1);
    }
  }
}

void StructExpr::print(int space_num) {}

void IfExpr::print(int space_num) {}

void LoopExpr::print(int space_num) {}

void WhileExpr::print(int space_num) {}

void BreakExpr::print(int space_num) {}

void ReturnExpr::print(int space_num) {}

void ContinueExpr::print(int space_num) {}

void UnaryExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "UnaryExpr " << op_ << '\n';
  expr_->print(space_num + 1);
}

void AssignExpr::print(int space_num) {}

void CastExpr::print(int space_num) {}

void CallExpr::print(int space_num) {}

void IndexExpr::print(int space_num) {}

void MemberExpr::print(int space_num) {}

void MethodCallExpr::print(int space_num) {}

void UnitType::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "UnitType\n";
}

void PathType::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "PathType\n";
  for (auto &segment : path_segments_)
  {
    segment.print(space_num + 1);
  }
}

void ReferenceType::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ReferenceType";
  if (lifetime_)
  {
    std::cout << ' ' << *lifetime_;
  }
  if (mut_)
  {
    std::cout << " mut";
  }
  std::cout << '\n';
  inner_type_->print(space_num + 1);
}

void ArrayType::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ArrayType\n";
  PrintSpace(space_num + 1);
  std::cout << "ElementType\n";
  type_->print(space_num + 2);
  PrintSpace(space_num + 1);
  std::cout << "Length\n";
  const_value_->print(space_num + 2);
}

std::ostream &operator<<(std::ostream &out, UnaryOperator op)
{
  switch (op)
  {
  case UnaryOperator::Negate:
    return out << '-';
  case UnaryOperator::Not:
    return out << '!';
  case UnaryOperator::Dereference:
    return out << '*';
  case UnaryOperator::Borrow:
    return out << '&';
  case UnaryOperator::BorrowMut:
    return out << "&mut";
  }

  return out;
}

std::ostream &operator<<(std::ostream &out, BinaryOperator op)
{
  switch (op)
  {
  case BinaryOperator::LogicalOr:
    return out << "||";
  case BinaryOperator::LogicalAnd:
    return out << "&&";
  case BinaryOperator::Equal:
    return out << "==";
  case BinaryOperator::NotEqual:
    return out << "!=";
  case BinaryOperator::Less:
    return out << '<';
  case BinaryOperator::LessEqual:
    return out << "<=";
  case BinaryOperator::Greater:
    return out << '>';
  case BinaryOperator::GreaterEqual:
    return out << ">=";
  case BinaryOperator::BitOr:
    return out << '|';
  case BinaryOperator::BitXor:
    return out << '^';
  case BinaryOperator::BitAnd:
    return out << '&';
  case BinaryOperator::ShiftLeft:
    return out << "<<";
  case BinaryOperator::ShiftRight:
    return out << ">>";
  case BinaryOperator::Add:
    return out << '+';
  case BinaryOperator::Subtract:
    return out << '-';
  case BinaryOperator::Multiply:
    return out << '*';
  case BinaryOperator::Divide:
    return out << '/';
  case BinaryOperator::Remainder:
    return out << '%';
  }
  return out;
}

void Crate::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "Crate\n";
  for (const auto &item : items_)
  {
    item->print(space_num + 1);
  }
}

FuncParam::FuncParam(std::string ident, bool mut, std::unique_ptr<Type> type)
    : ident_(std::move(ident)), mut_(mut), type_(std::move(type))
{
}

void FuncParam::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "FuncParam\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
  if (mut_)
  {
    PrintSpace(space_num + 1);
    std::cout << "mutable\n";
  }
  if (type_)
  {
    type_->print(space_num + 1);
  }
}

void SelfParam::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "SelfParam\n";
  PrintSpace(space_num + 1);
  if (is_reference_)
  {
    std::cout << '&';
  }
  if (mut_)
  {
    std::cout << "mut ";
  }
  std::cout << "self\n";
}

void FuncItem::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "FuncItem\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
  if (self_param_)
  {
    self_param_->print(space_num + 1);
  }
  for (auto &param : func_params_)
  {
    param.print(space_num + 1);
  }
  if (return_type_)
  {
    PrintSpace(space_num + 1);
    std::cout << "ReturnType\n";
    return_type_->print(space_num + 2);
  }
  block_expr_->print(space_num + 1);
}

void BlockExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "BlockExpr\n";
  for (auto &stmt : stmts_)
  {
    stmt->print(space_num + 1);
  }
}

void LetStmt::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "LetStmt\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
  if (mut_)
  {
    PrintSpace(space_num + 1);
    std::cout << "mutable\n";
  }
  if (type_)
  {
    type_->print(space_num + 1);
  }
  expr_->print(space_num + 1);
}

void BinaryExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "BinaryExpr " << op_ << '\n';
  lhs_->print(space_num + 1);
  rhs_->print(space_num + 1);
}

void LiteralExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "LieralExpr\n";
  PrintSpace(space_num + 1);
  if (is_int_)
  {
    std::cout << int_value_ << '\n';
  }
  else
  {
    std::cout << ((bool_value_) ? "TRUE" : "FALSE") << '\n';
  }
}
