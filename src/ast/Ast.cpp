#include "Ast.h"

GenericArg::GenericArg(std::unique_ptr<Type> type)
    : type_(std::move(type))
{
}

void GenericArg::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "GenericArg type\n";
  type_->print(space_num + 1);
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

void ConstItem::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ConstItem\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
  type_->print(space_num + 1);
  const_value_->print(space_num + 1);
}

void ImplItem::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ImplItem\n";
  type_->print(space_num + 1);
  for (const auto &item : items_)
  {
    item->print(space_num + 1);
  }
}

StructField::StructField(std::string ident, std::unique_ptr<Type> type)
    : ident_(std::move(ident)), type_(std::move(type))
{
}

void StructField::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "StructField\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
  type_->print(space_num + 1);
}

void StructItem::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "StructItem\n";
  PrintSpace(space_num + 1);
  std::cout << ident_ << '\n';
  for (auto derive : derives_)
  {
    PrintSpace(space_num + 1);
    std::cout << "Derive ";
    switch (derive)
    {
    case DeriveKind::Copy: std::cout << "Copy"; break;
    case DeriveKind::Clone: std::cout << "Clone"; break;
    case DeriveKind::PartialEq: std::cout << "PartialEq"; break;
    case DeriveKind::Eq: std::cout << "Eq"; break;
    }
    std::cout << '\n';
  }
  for (auto &field : fields_)
  {
    field.print(space_num + 1);
  }
}

void ExprStmt::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ExprStmt\n";
  expr_->print(space_num + 1);
}

void UnitExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "UnitExpr\n";
}

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

void StructExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "StructExpr\n";
  path_->print(space_num + 1);
  for (auto &field : struct_fields_)
  {
    PrintSpace(space_num + 1);
    std::cout << "StructExprField " << field.name_ << '\n';
    field.expr_->print(space_num + 2);
  }
}

void IfExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "IfExpr\n";
  PrintSpace(space_num + 1);
  std::cout << "Condition\n";
  condition_expr_->print(space_num + 2);
  PrintSpace(space_num + 1);
  std::cout << "Then\n";
  block_expr_->print(space_num + 2);
  if (has_else_)
  {
    PrintSpace(space_num + 1);
    std::cout << "Else\n";
    if (is_else_if_)
    {
      else_if_expr_->print(space_num + 2);
    }
    else
    {
      else_block_expr_->print(space_num + 2);
    }
  }
}

void LoopExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "LoopExpr\n";
  block_expr_->print(space_num + 1);
}

void WhileExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "WhileExpr\n";
  PrintSpace(space_num + 1);
  std::cout << "Condition\n";
  condition_expr_->print(space_num + 2);
  block_expr_->print(space_num + 1);
}

void BreakExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "BreakExpr\n";
  if (value_)
  {
    value_->print(space_num + 1);
  }
}

void ReturnExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ReturnExpr\n";
  if (value_)
  {
    value_->print(space_num + 1);
  }
}

void ContinueExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "ContinueExpr\n";
}

void UnaryExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "UnaryExpr " << op_ << '\n';
  expr_->print(space_num + 1);
}

void AssignExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "AssignExpr ";
  switch (op_)
  {
  case AssignmentOperator::Assign: std::cout << "="; break;
  case AssignmentOperator::AddAssign: std::cout << "+="; break;
  case AssignmentOperator::SubtractAssign: std::cout << "-="; break;
  case AssignmentOperator::MultiplyAssign: std::cout << "*="; break;
  case AssignmentOperator::DivideAssign: std::cout << "/="; break;
  case AssignmentOperator::RemainderAssign: std::cout << "%="; break;
  case AssignmentOperator::BitAndAssign: std::cout << "&="; break;
  case AssignmentOperator::BitOrAssign: std::cout << "|="; break;
  case AssignmentOperator::BitXorAssign: std::cout << "^="; break;
  case AssignmentOperator::ShiftLeftAssign: std::cout << "<<="; break;
  case AssignmentOperator::ShiftRightAssign: std::cout << ">>="; break;
  }
  std::cout << '\n';
  lhs_->print(space_num + 1);
  rhs_->print(space_num + 1);
}

void CastExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "CastExpr\n";
  expr_->print(space_num + 1);
  type_->print(space_num + 1);
}

void CallExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "CallExpr\n";
  callee_->print(space_num + 1);
  for (auto &argument : arguments_)
  {
    argument->print(space_num + 1);
  }
}

void IndexExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "IndexExpr\n";
  base_->print(space_num + 1);
  index_->print(space_num + 1);
}

void MemberExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "MemberExpr\n";
  base_->print(space_num + 1);
  PrintSpace(space_num + 1);
  std::cout << member_ << '\n';
}

void MethodCallExpr::print(int space_num)
{
  PrintSpace(space_num);
  std::cout << "MethodCallExpr\n";
  base_->print(space_num + 1);
  method_.print(space_num + 1);
  for (auto &argument : arguments_)
  {
    argument->print(space_num + 1);
  }
}

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
  if (tail_expr_)
  {
    PrintSpace(space_num + 1);
    std::cout << "TailExpr\n";
    tail_expr_->print(space_num + 2);
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
  std::cout << "LiteralExpr\n";
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
