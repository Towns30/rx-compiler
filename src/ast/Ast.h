#pragma once

#include "../Utils.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct SourceSpan
{
  // [begin, end)
  std::size_t begin;
  std::size_t end;
};

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

enum class DeriveKind
{
  Copy,
  Clone,
  PartialEq,
  Eq
};

struct StructField
{
  std::optional<SourceSpan> span_;
  std::string ident_;
  std::optional<SourceSpan> ident_span_;
  std::unique_ptr<Type> type_;

  StructField(std::string ident, std::unique_ptr<Type> type);

  void print(int space_num);
};

struct FuncParam
{
  std::optional<SourceSpan> span_;
  std::string ident_;
  std::optional<SourceSpan> ident_span_;
  bool mut_;
  std::unique_ptr<Type> type_;

  FuncParam(std::string ident, bool mut, std::unique_ptr<Type> type);

  void print(int space_num);
};

struct SelfParam
{
  std::optional<SourceSpan> span_;
  std::optional<SourceSpan> self_span_;
  bool is_reference_;
  bool mut_;

  SelfParam(bool is_reference, bool mut)
      : is_reference_(is_reference), mut_(mut)
  {
  }

  void print(int space_num);
};

struct GenericArg
{
  std::optional<SourceSpan> span_;
  std::unique_ptr<Type> type_;

  explicit GenericArg(std::unique_ptr<Type> type);

  void print(int space_num);
};

struct PathSegment
{
  std::optional<SourceSpan> span_;
  std::string name_; // Vec、new、self、Self
  std::optional<SourceSpan> name_span_;
  std::vector<GenericArg> generic_args_;

  PathSegment(std::string name, std::vector<GenericArg> generic_args = {})
      : name_(std::move(name)), generic_args_(std::move(generic_args))
  {
  }

  void print(int space_num);
};

struct Path
{
  std::optional<SourceSpan> span_;
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
  // 未填入位置或没有直接对应的源码时为 std::nullopt。
  std::optional<SourceSpan> span_;
  virtual ~ASTNode() = default;
  virtual void print(int space_num) = 0;
};

class Crate : public ASTNode
{
public:
  std::vector<std::unique_ptr<Item>> items_;

  Crate() = default;
  Crate(std::vector<std::unique_ptr<Item>> items) : items_(std::move(items)) {}

  Crate(const Crate &) = delete;
  Crate &operator=(const Crate &) = delete;
  Crate(Crate &&) noexcept = default;
  Crate &operator=(Crate &&) noexcept = default;

  void print(int space_num) override;
};

class Item : public ASTNode
{
};

class FuncItem : public Item
{
public:
  std::string ident_;
  std::optional<SourceSpan> ident_span_;
  std::optional<SelfParam> self_param_;
  std::vector<FuncParam> func_params_;
  std::unique_ptr<Type> return_type_;
  std::unique_ptr<BlockExpr> block_expr_;

  FuncItem(std::string ident, std::optional<SelfParam> self_param,
           std::vector<FuncParam> func_params,
           std::unique_ptr<Type> return_type,
           std::unique_ptr<BlockExpr> block_expr)
      : ident_(std::move(ident)), self_param_(std::move(self_param)),
        func_params_(std::move(func_params)),
        return_type_(std::move(return_type)), block_expr_(std::move(block_expr))
  {
  }

  void print(int space_num) override;
};

class ConstItem : public Item
{
public:
  std::string ident_;
  std::optional<SourceSpan> ident_span_;
  std::unique_ptr<Type> type_;
  std::unique_ptr<Expr> const_value_;

  ConstItem(std::string ident, std::unique_ptr<Type> type,
            std::unique_ptr<Expr> const_value)
      : ident_(std::move(ident)), type_(std::move(type)),
        const_value_(std::move(const_value))
  {
  }

  void print(int space_num) override;
};

class ImplItem : public Item
{
public:
  std::unique_ptr<Type> type_;
  std::vector<std::unique_ptr<Item>> items_;

  ImplItem(std::unique_ptr<Type> type, std::vector<std::unique_ptr<Item>> items)
      : type_(std::move(type)), items_(std::move(items))
  {
  }

  void print(int space_num) override;
};

class StructItem : public Item
{
public:
  std::string ident_;
  std::optional<SourceSpan> ident_span_;
  std::vector<DeriveKind> derives_;
  std::vector<StructField> fields_;

  StructItem(std::string ident, std::vector<DeriveKind> derives,
             std::vector<StructField> fields)
      : ident_(std::move(ident)), derives_(std::move(derives)),
        fields_(std::move(fields))
  {
  }

  void print(int space_num) override;
};

class Stmt : public ASTNode
{
};

class LetStmt : public Stmt
{
public:
  std::string ident_;
  std::optional<SourceSpan> ident_span_;
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
  std::unique_ptr<Expr> expr_;

  ExprStmt(std::unique_ptr<Expr> expr) : expr_(std::move(expr)) {}

  void print(int space_num) override;
};

class Expr : public ASTNode
{
};

class LiteralExpr : public Expr
{
public:
  // 字面量 token 的范围，包含整数后缀，不包含外围括号或一元负号。
  std::optional<SourceSpan> literal_span_;
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
  std::vector<PathSegment> path_segments_;

  PathExpr(std::vector<PathSegment> path_segments)
      : path_segments_(std::move(path_segments))
  {
  }

  void print(int space_num) override;
};

class ArrayExpr : public Expr
{
public:
  bool is_repeat_;
  std::vector<std::unique_ptr<Expr>> exprs_;
  std::unique_ptr<Expr> repeat_expr_;
  std::unique_ptr<Expr> repeat_count_;

  ArrayExpr(std::vector<std::unique_ptr<Expr>> exprs)
      : is_repeat_(false), exprs_(std::move(exprs))
  {
  }

  ArrayExpr(std::unique_ptr<Expr> repeat_expr,
            std::unique_ptr<Expr> repeat_count)
      : is_repeat_(true), repeat_expr_(std::move(repeat_expr)),
        repeat_count_(std::move(repeat_count))
  {
  }

  void print(int space_num) override;
};

class StructExprField
{
public:
  std::optional<SourceSpan> span_;
  std::string name_;
  std::optional<SourceSpan> name_span_;
  std::unique_ptr<Expr> expr_;

  StructExprField(std::string name, std::unique_ptr<Expr> expr)
      : name_(std::move(name)), expr_(std::move(expr))
  {
  }
};

class StructExpr : public Expr
{
public:
  std::unique_ptr<PathExpr> path_;
  std::vector<StructExprField> struct_fields_;

  StructExpr(std::unique_ptr<PathExpr> path,
             std::vector<StructExprField> struct_fields)
      : path_(std::move(path)), struct_fields_(std::move(struct_fields))
  {
  }

  void print(int space_num) override;
};

class BlockExpr : public Expr
{
public:
  std::vector<std::unique_ptr<Stmt>> stmts_;
  std::unique_ptr<Expr> tail_expr_;

  BlockExpr(std::vector<std::unique_ptr<Stmt>> stmts,
            std::unique_ptr<Expr> tail_expr)
      : stmts_(std::move(stmts)), tail_expr_(std::move(tail_expr))
  {
  }

  void print(int space_num) override;
};

class IfExpr : public Expr
{
public:
  std::unique_ptr<Expr> condition_expr_;
  std::unique_ptr<BlockExpr> block_expr_;
  bool has_else_;
  bool is_else_if_;
  std::unique_ptr<BlockExpr> else_block_expr_;
  std::unique_ptr<IfExpr> else_if_expr_;

  IfExpr(std::unique_ptr<Expr> condition_expr,
         std::unique_ptr<BlockExpr> block_expr, bool has_else = false,
         bool is_else_if = false,
         std::unique_ptr<BlockExpr> else_block_expr = nullptr,
         std::unique_ptr<IfExpr> else_if_expr = nullptr)
      : condition_expr_(std::move(condition_expr)),
        block_expr_(std::move(block_expr)), has_else_(has_else),
        is_else_if_(is_else_if), else_block_expr_(std::move(else_block_expr)),
        else_if_expr_(std::move(else_if_expr))
  {
  }

  void print(int space_num) override;
};

class LoopExpr : public Expr
{
public:
  std::unique_ptr<BlockExpr> block_expr_;

  explicit LoopExpr(std::unique_ptr<BlockExpr> block_expr)
      : block_expr_(std::move(block_expr))
  {
  }

  void print(int space_num) override;
};

class WhileExpr : public Expr
{
public:
  std::unique_ptr<Expr> condition_expr_;
  std::unique_ptr<BlockExpr> block_expr_;

  WhileExpr(std::unique_ptr<Expr> condition_expr,
            std::unique_ptr<BlockExpr> block_expr)
      : condition_expr_(std::move(condition_expr)),
        block_expr_(std::move(block_expr))
  {
  }

  void print(int space_num) override;
};

class BreakExpr : public Expr
{
public:
  std::unique_ptr<Expr> value_;

  explicit BreakExpr(std::unique_ptr<Expr> value = nullptr)
      : value_(std::move(value))
  {
  }

  void print(int space_num) override;
};

class ReturnExpr : public Expr
{
public:
  std::unique_ptr<Expr> value_;

  explicit ReturnExpr(std::unique_ptr<Expr> value = nullptr)
      : value_(std::move(value))
  {
  }

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
  std::unique_ptr<Expr> expr_;
  std::unique_ptr<Type> type_;

  CastExpr(std::unique_ptr<Expr> expr, std::unique_ptr<Type> type)
      : expr_(std::move(expr)), type_(std::move(type))
  {
  }

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
  std::optional<SourceSpan> member_span_;

  MemberExpr(std::unique_ptr<Expr> base, std::string member)
      : base_(std::move(base)), member_(std::move(member))
  {
  }

  void print(int space_num) override;
};

class MethodCallExpr : public Expr
{
public:
  std::unique_ptr<Expr> base_;
  PathSegment method_;
  std::vector<std::unique_ptr<Expr>> arguments_;

  MethodCallExpr(std::unique_ptr<Expr> base, PathSegment method,
                 std::vector<std::unique_ptr<Expr>> arguments)
      : base_(std::move(base)), method_(std::move(method)),
        arguments_(std::move(arguments))
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
  std::vector<PathSegment> path_segments_;

  PathType(std::vector<PathSegment> path_segments)
      : path_segments_(std::move(path_segments))
  {
  }

  void print(int space_num) override;
};

class ReferenceType : public Type
{
public:
  std::unique_ptr<Type> inner_type_;
  bool mut_;

  ReferenceType(std::unique_ptr<Type> inner_type, bool mut = false)
      : inner_type_(std::move(inner_type)), mut_(mut)
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
