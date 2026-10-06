#include "Ast.h"

void UseItem::print(int space_num) {}

void ConstItem::print(int space_num) {}

void ImplItem::print(int space_num) {}

void StructItem::print(int space_num) {}

void ExprStmt::print(int space_num) {}

void UnitExpr::print(int space_num) {}

void PathExpr::print(int space_num) {}

void ArrayExpr::print(int space_num) {}

void StructExpr::print(int space_num) {}

void IfExpr::print(int space_num) {}

void LoopExpr::print(int space_num) {}

void WhileExpr::print(int space_num) {}

void BreakExpr::print(int space_num) {}

void ReturnExpr::print(int space_num) {}

void ContinueExpr::print(int space_num) {}

void UnaryExpr::print(int space_num) {}

void AssignExpr::print(int space_num) {}

void CastExpr::print(int space_num) {}

void CallExpr::print(int space_num) {}

void IndexExpr::print(int space_num) {}

void MemberExpr::print(int space_num) {}

void UnitType::print(int space_num) {}

void PathType::print(int space_num) {}

void ReferenceType::print(int space_num) {}

void ArrayType::print(int space_num) {}

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

void FuncItem::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "FuncItem\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
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
