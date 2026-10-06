#pragma once

#include "../Utils.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

inline void PrintSpace(int num)
{
  for (int i = 0; i < num; i++)
  {
    std::cout << "  ";
  }
}

enum class AssignmentOperator
{
  Assign,          // =
  AddAssign,       // +=
  SubtractAssign,  // -=
  MultiplyAssign,  // *=
  DivideAssign,    // /=
  RemainderAssign, // %=
  BitAndAssign,    // &=
  BitOrAssign,     // |=
  BitXorAssign,    // ^=
  ShiftLeftAssign, // <<=
  ShiftRightAssign // >>=
};

enum class UnaryOperator
{
  Negate,      // -x
  Not,         // !x
  Dereference, // *x
  Borrow,      // &x
  BorrowMut    // &mut x
};

std::ostream &operator<<(std::ostream &out, UnaryOperator op);

enum class BinaryOperator
{
  LogicalOr,    // ||
  LogicalAnd,   // &&
  Equal,        // ==
  NotEqual,     // !=
  Less,         // <
  LessEqual,    // <=
  Greater,      // >
  GreaterEqual, // >=
  BitOr,        // |
  BitXor,       // ^
  BitAnd,       // &
  ShiftLeft,    // <<
  ShiftRight,   // >>
  Add,          // +
  Subtract,     // -
  Multiply,     // *
  Divide,       // /
  Remainder     // %
};

std::ostream &operator<<(std::ostream &out, BinaryOperator op);

class Item;
class Crate;
class Stmt;
class Expr;
class BlockExpr;
class Type;
class ConstItem;

struct GenericArg
{
  bool is_type_;
  std::unique_ptr<Type> type_;
  std::optional<std::string> life_time_;

  GenericArg(bool is_type, std::unique_ptr<Type> type,
             std::optional<std::string> life_time);

  void print(int space_num);
};

struct PathSegment
{
  std::string name_; // Vec、new、self、Self
  std::vector<GenericArg> generic_args_;

  PathSegment(std::string name,
                       std::vector<GenericArg> generic_args = {})
      : name_(std::move(name)), generic_args_(std::move(generic_args))
  {
  }

  void print(int space_num);
};

struct Path
{
  bool absolute_ = false; // 是否以 :: 开头
  std::vector<PathSegment> segments_;

  Path(std::vector<PathSegment> segments, bool absolute = false)
      : absolute_(absolute), segments_(std::move(segments))
  {
  }

  void print(int space_num);
};

class ASTNode
{
public:
  virtual ~ASTNode() = default;
  virtual void print(int space_num) = 0;
};

class Crate : public ASTNode
{
public:
  std::vector<std::unique_ptr<Item>> items_;

  Crate() = default;
  Crate(std::vector<std::unique_ptr<Item>> items)
      : items_(std::move(items))
  {
  }

  Crate(const Crate &) = delete;
  Crate &operator=(const Crate &) = delete;
  Crate(Crate &&) noexcept = default;
  Crate &operator=(Crate &&) noexcept = default;

  void print(int space_num) override;
};

class Item : public ASTNode
{
};

class UseItem : public Item
{
public:
  void print(int space_num) override;
};

class FuncItem : public Item
{
public:
  std::string ident_;
  std::unique_ptr<BlockExpr> block_expr_;

  FuncItem(std::string ident, std::unique_ptr<BlockExpr> block_expr)
      : ident_(std::move(ident)), block_expr_(std::move(block_expr))
  {
  }

  void print(int space_num) override;
};

class ConstItem : public Item
{
public:
  std::string ident_;
  std::unique_ptr<Type> type_;
  std::unique_ptr<Expr> const_value_;

  void print(int space_num) override;
};

class ImplItem : public Item
{
public:
  void print(int space_num) override;
};

class StructItem : public Item
{
public:
  void print(int space_num) override;
};

class Stmt : public ASTNode
{
};

class LetStmt : public Stmt
{
public:
  std::string ident_;
  bool mut_;
  std::unique_ptr<Type> type_;
  std::unique_ptr<Expr> expr_;

  LetStmt(std::string ident, bool mut, std::unique_ptr<Type> type,
          std::unique_ptr<Expr> expr)
      : ident_(std::move(ident)), mut_(mut), type_(std::move(type)),
        expr_(std::move(expr))
  {
  }

  void print(int space_num) override;
};

class ExprStmt : public Stmt
{
public:
  void print(int space_num) override;
};

class Expr : public ASTNode
{
};

class LiteralExpr : public Expr
{
public:
  bool is_int_;
  bool bool_value_;
  std::uint64_t int_value_;
  IntegerType int_type_;
  

  LiteralExpr(bool is_int, bool bool_value, std::uint64_t int_value,
              IntegerType int_type)
      : bool_value_(bool_value), int_value_(int_value), int_type_(int_type),
        is_int_(is_int)
  {
  }

  void print(int space_num) override;
};

class UnitExpr : public Expr
{
public:
  void print(int space_num) override;
};

class PathExpr : public Expr
{
public:
  std::vector<PathSegment> path_;

  explicit PathExpr(std::vector<PathSegment> path)
      : path_(std::move(path))
  {
  }

  void print(int space_num) override;
};

class ArrayExpr : public Expr
{
public:
  void print(int space_num) override;
};

class StructExpr : public Expr
{
public:
  void print(int space_num) override;
};

class BlockExpr : public Expr
{
public:
  std::vector<std::unique_ptr<Stmt>> stmts_;

  BlockExpr(std::vector<std::unique_ptr<Stmt>> stmts)
      : stmts_(std::move(stmts))
  {
  }

  void print(int space_num) override;
};

class IfExpr : public Expr
{
public:
  void print(int space_num) override;
};

class LoopExpr : public Expr
{
public:
  void print(int space_num) override;
};

class WhileExpr : public Expr
{
public:
  void print(int space_num) override;
};

class BreakExpr : public Expr
{
public:
  void print(int space_num) override;
};

class ReturnExpr : public Expr
{
public:
  void print(int space_num) override;
};

class ContinueExpr : public Expr
{
public:
  void print(int space_num) override;
};

class UnaryExpr : public Expr
{
public:
  UnaryOperator op_;
  std::unique_ptr<Expr> expr_;

  UnaryExpr(UnaryOperator op, std::unique_ptr<Expr> expr)
      : op_(op), expr_(std::move(expr))
  {
  }

  void print(int space_num) override;
};

class BinaryExpr : public Expr
{
public:
  BinaryOperator op_;
  std::unique_ptr<Expr> lhs_;
  std::unique_ptr<Expr> rhs_;

  BinaryExpr(BinaryOperator op, std::unique_ptr<Expr> lhs,
             std::unique_ptr<Expr> rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs))
  {
  }

  void print(int space_num) override;
};

class AssignExpr : public Expr
{
public:
  AssignmentOperator op_;
  std::unique_ptr<Expr> lhs_;
  std::unique_ptr<Expr> rhs_;

  AssignExpr(AssignmentOperator op, std::unique_ptr<Expr> lhs,
             std::unique_ptr<Expr> rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs))
  {
  }

  void print(int space_num) override;
};

class CastExpr : public Expr
{
public:
  void print(int space_num) override;
};

class CallExpr : public Expr
{
public:
  std::unique_ptr<Expr> callee_;
  std::vector<std::unique_ptr<Expr>> arguments_;

  CallExpr(std::unique_ptr<Expr> callee,
           std::vector<std::unique_ptr<Expr>> arguments)
      : callee_(std::move(callee)), arguments_(std::move(arguments))
  {
  }

  void print(int space_num) override;
};

class IndexExpr : public Expr
{
public:
  std::unique_ptr<Expr> base_;
  std::unique_ptr<Expr> index_;

  IndexExpr(std::unique_ptr<Expr> base, std::unique_ptr<Expr> index)
      : base_(std::move(base)), index_(std::move(index))
  {
  }

  void print(int space_num) override;
};

class MemberExpr : public Expr
{
public:
  std::unique_ptr<Expr> base_;
  std::string member_;

  MemberExpr(std::unique_ptr<Expr> base, std::string member)
      : base_(std::move(base)), member_(std::move(member))
  {
  }

  void print(int space_num) override;
};

class Type : public ASTNode
{
};

class UnitType : public Type
{
public:
  UnitType() = default;

  void print(int space_num) override;
};

class PathType : public Type
{
public:
  std::vector<PathSegment> path_;

  PathType(std::vector<PathSegment> path)
      : path_(std::move(path))
  {
  }

  void print(int space_num) override;
};

class ReferenceType : public Type
{
public:
  std::unique_ptr<Type> inner_type_;
  bool mut_;
  std::optional<std::string> lifetime_;

  ReferenceType(std::unique_ptr<Type> inner_type, bool mut = false,
                         std::optional<std::string> lifetime = std::nullopt)
      : inner_type_(std::move(inner_type)), mut_(mut),
        lifetime_(std::move(lifetime))
  {
  }

  void print(int space_num) override;
};

class ArrayType : public Type
{
public:
  std::unique_ptr<Type> type_;
  std::unique_ptr<Expr> const_value_;

  ArrayType(std::unique_ptr<Type> type, std::unique_ptr<Expr> const_value)
      : type_(std::move(type)), const_value_(std::move(const_value))
  {
  }

  void print(int space_num) override;
};
