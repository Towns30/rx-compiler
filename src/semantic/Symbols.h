#pragma once
#include "Types.h"
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using ConstValue = std::variant<std::int64_t, bool>;

struct DerefInfo
{
  TypeId type_id_;
  bool through_shared_;
};

struct ExprInfo
{
  TypeId type_id_;
  bool is_place_;
  bool mut_;
  bool diverges_ = false;
  bool through_shared_ = false;
};

enum class LoopKind
{
  Loop,
  While,
  WhileCondition
};

struct LoopInfo
{
  LoopKind kind_;
  std::optional<TypeId> expected_type_id_ = std::nullopt;
  std::vector<ExprInfo> break_values_;
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

enum class ReceiverAdjustmentKind
{
  Direct,
  Borrow,
  BorrowMut
};

struct MethodInfo
{
  TypeId receiver_type_id_;
  std::vector<TypeId> args_type_id_;
  TypeId return_type_id_;
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
