#pragma once

#include "Scope.h"
#include "SemanticContext.h"
#include "Symbols.h"
#include "TypeResolver.h"

class BodyChecker
{
public:
  bool checkAll(const Crate *crate, SemanticContext &semantic_ctx);

private:
  std::vector<LoopInfo> loop_stack_;
  std::optional<std::size_t> getLoopTarget();
  std::optional<MethodInfo> findMethod(TypeId receiver_type_id,
                                       const std::string &method_name,
                                       SemanticContext &semantic_ctx);
  bool canCast(TypeId source_type_id, TypeId target_type_id,
               SemanticContext &semantic_ctx);
  std::optional<DerefInfo> dereferenceOnce(TypeId type_id,
                                           SemanticContext &semantic_ctx);
  bool canCoerce(TypeId source_type_id, TypeId target_type_id,
                 SemanticContext &semantic_ctx);
  std::optional<TypeId> getLubTypeId(const std::vector<ExprInfo> &expr_infos,
                                    SemanticContext &semantic_ctx);
  bool checkFunction(const FuncInfo &func_info, SemanticContext &semantic_ctx);
  std::optional<ExprInfo> checkExpr(const Expr *expr,
                                    SemanticContext &semantic_ctx, Scope &scope,
                                    std::optional<TypeId> expected_type_id,
                                    const FuncInfo &func_info);
  std::optional<TypeId> getIntegerTypeId(IntegerType integer_type);
  std::optional<ExprInfo>
  checkLiteralExpr(const LiteralExpr *literal_expr,
                   SemanticContext &semantic_ctx, Scope &scope,
                   std::optional<TypeId> expected_type_id,
                   const FuncInfo &func_info);
  std::optional<ExprInfo> checkUnitExpr(const UnitExpr *unit_expr,
                                        SemanticContext &semantic_ctx,
                                        Scope &scope,
                                        std::optional<TypeId> expected_type_id,
                                        const FuncInfo &func_info);
  std::optional<ExprInfo> checkArrayExpr(const ArrayExpr *array_expr,
                                         SemanticContext &semantic_ctx,
                                         Scope &scope,
                                         std::optional<TypeId> expected_type_id,
                                         const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkStructExpr(const StructExpr *struct_expr, SemanticContext &semantic_ctx,
                  Scope &scope, std::optional<TypeId> expected_type_id,
                  const FuncInfo &func_info);
  std::optional<ExprInfo> checkIfExpr(const IfExpr *if_expr,
                                      SemanticContext &semantic_ctx,
                                      Scope &scope,
                                      std::optional<TypeId> expected_type_id,
                                      const FuncInfo &func_info);
  std::optional<ExprInfo> checkBreakExpr(const BreakExpr *break_expr,
                                         SemanticContext &semantic_ctx,
                                         Scope &scope,
                                         std::optional<TypeId> expected_type_id,
                                         const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkReturnExpr(const ReturnExpr *return_expr, SemanticContext &semantic_ctx,
                  Scope &scope, std::optional<TypeId> expected_type_id,
                  const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkContinueExpr(const ContinueExpr *continue_expr,
                    SemanticContext &semantic_ctx, Scope &scope,
                    std::optional<TypeId> expected_type_id,
                    const FuncInfo &func_info);
  std::optional<ExprInfo> checkUnaryExpr(const UnaryExpr *unary_expr,
                                         SemanticContext &semantic_ctx,
                                         Scope &scope,
                                         std::optional<TypeId> expected_type_id,
                                         const FuncInfo &func_info);
  std::optional<ExprInfo> checkPathExpr(const PathExpr *path_expr,
                                        SemanticContext &semantic_ctx,
                                        Scope &scope,
                                        std::optional<TypeId> expected_type_id,
                                        const FuncInfo &func_info);
  std::optional<ExprInfo> checkBlockExpr(const BlockExpr *block_expr,
                                         SemanticContext &semantic_ctx,
                                         Scope &scope,
                                         std::optional<TypeId> expected_type_id,
                                         const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkBinaryExpr(const BinaryExpr *binary_expr, SemanticContext &semantic_ctx,
                  Scope &scope, std::optional<TypeId> expected_type_id,
                  const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkAssignExpr(const AssignExpr *assign_expr, SemanticContext &semantic_ctx,
                  Scope &scope, std::optional<TypeId> expected_type_id,
                  const FuncInfo &func_info);
  std::optional<ExprInfo> checkCastExpr(const CastExpr *cast_expr,
                                        SemanticContext &semantic_ctx,
                                        Scope &scope,
                                        std::optional<TypeId> expected_type_id,
                                        const FuncInfo &func_info);
  std::optional<ExprInfo> checkCallExpr(const CallExpr *call_expr,
                                        SemanticContext &semantic_ctx,
                                        Scope &scope,
                                        std::optional<TypeId> expected_type_id,
                                        const FuncInfo &func_info);
  std::optional<ExprInfo> checkIndexExpr(const IndexExpr *index_expr,
                                         SemanticContext &semantic_ctx,
                                         Scope &scope,
                                         std::optional<TypeId> expected_type_id,
                                         const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkMemberExpr(const MemberExpr *member_expr, SemanticContext &semantic_ctx,
                  Scope &scope, std::optional<TypeId> expected_type_id,
                  const FuncInfo &func_info);
  std::optional<ExprInfo>
  checkMethodCallExpr(const MethodCallExpr *method_call_expr,
                      SemanticContext &semantic_ctx, Scope &scope,
                      std::optional<TypeId> expected_type_id,
                      const FuncInfo &func_info);
  std::optional<ExprInfo> checkLoopExpr(const LoopExpr *loop_expr,
                                        SemanticContext &semantic_ctx,
                                        Scope &scope,
                                        std::optional<TypeId> expected_type_id,
                                        const FuncInfo &func_info);
  std::optional<ExprInfo> checkWhileExpr(const WhileExpr *while_expr,
                                         SemanticContext &semantic_ctx,
                                         Scope &scope,
                                         std::optional<TypeId> expected_type_id,
                                         const FuncInfo &func_info);
};
