#pragma once
#include "Types.h"
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace semantic
{

using ConstValue = std::variant<std::int64_t, bool>;
using LocalId = std::size_t;

enum class DerefKind
{
  Reference,
  Box
};

struct DerefInfo
{
  TypeId type_id_;
  bool through_shared_;
};

enum class ReceiverAdjustmentKind
{
  Direct,
  Borrow,
  BorrowMut
};

enum class CallTargetKind
{
  Function,
  BoxNew,
  VecNew,
  ArrayLen,
  VecLen,
  VecIsEmpty,
  VecPush,
  VecRemove,
  Clone
};

struct CallTarget
{
  CallTargetKind kind_;
  std::optional<FunctionId> function_id_ = std::nullopt;
  std::optional<TypeId> owner_type_id_ = std::nullopt; // 调用内置函数的类，如i32::Clone(), 不考虑struct里定义的func
};

struct ExprInfo
{
  TypeId type_id_;
  bool is_place_;
  bool mut_;
  bool diverges_ = false;
  bool through_shared_ = false;
  std::optional<LocalId> local_id_ = std::nullopt;
  std::optional<std::size_t> const_id_ = std::nullopt; // 关联常量id
  std::optional<TypeId> coerced_type_id_ = std::nullopt; // 隐式转换目标
  std::vector<DerefKind> coercion_derefs_; // 隐式转换解引用步骤
  std::vector<DerefKind> base_derefs_; // 基对象的自动解引用步骤
  std::optional<ReceiverAdjustmentKind> receiver_adjustment_ = std::nullopt; // 接收者自动借用
  std::optional<std::size_t> member_id_ = std::nullopt; // memberExpr的访问字段编号
  std::vector<std::size_t> struct_field_ids_; // 初始化项按源码顺序对应的声明字段编号
  std::optional<CallTarget> call_target_ = std::nullopt; // 调用函数信息
};

struct FuncLocalIds
{
  // 与 IR 参数顺序一致，有 self 时放在首位。
  std::vector<LocalId> param_local_ids_;
};

enum class LoopKind
{
  Loop,
  While,
  WhileCondition
};

struct BreakValueInfo
{
  const Expr *expr_;
  ExprInfo expr_info_;
};

struct LoopInfo
{
  LoopKind kind_;
  std::optional<TypeId> expected_type_id_ = std::nullopt;
  std::vector<BreakValueInfo> break_values_;
};

enum class ConstEvaluateStatus {
  Wait,
  Evaluating,
  Done
};

enum class VisitState
{
  Unvisited,
  Visiting,
  Done
};

enum class ValueKind
{
  Function,
  Constant
};

struct FuncConstSymbol
{
  ValueKind kind_;
  std::size_t index_;
};

class FieldInfo
{
public:
  std::string name_;
  TypeId type_id_;
};

class VarInfo
{
public:
  std::string name_;
  TypeId type_id_;
  bool mut_;
  std::optional<LocalId> local_id_ = std::nullopt;
};

class FuncInfo
{
public:
  std::string name_;
  std::vector<TypeId> args_type_id_;
  TypeId return_type_id_;
  std::optional<SelfParam> self_param_;
  std::optional<StructId> owner_struct_id_;
  bool is_builtin_ = false;
  const FuncItem *declaration_ = nullptr;
};

struct MethodInfo
{
  TypeId receiver_type_id_;
  std::vector<TypeId> args_type_id_;
  TypeId return_type_id_;
  CallTarget call_target_;
};

class StructInfo
{
public:
  std::string name_;
  std::optional<TypeId> type_id_;
  std::vector<FieldInfo> members_;
  std::vector<DeriveKind> derives_;
  std::unordered_map<std::string, FuncConstSymbol> associate_items_;
  const StructItem *declaration_ = nullptr;
};

class ConstInfo
{
public:
  std::string name_;
  std::optional<TypeId> type_id_ = std::nullopt;
  std::optional<StructId> owner_struct_id_;
  std::optional<ConstValue> value_;
  ConstEvaluateStatus status_;
  const ConstItem *declaration_ = nullptr;
};

} // namespace semantic
