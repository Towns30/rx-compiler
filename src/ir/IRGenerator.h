#pragma once

#include "../semantic/SemanticContext.h"
#include "IRSymbol.h"
#include "IRTypeTable.h"
#include "Type.h"
#include <optional>
#include <unordered_map>

namespace ir
{

class IRGenerator
{
public:
  Module generateIR(semantic::SemanticContext &semantic_ctx);
  IRGenerator(const Crate *crate) : crate_(crate) {}

private:
  // 约定若终止operand为空
  ExprResult generateExpr(const Expr *expr,
                          semantic::SemanticContext &semantic_ctx);
  // 返回位置地址；值表达式会临时物化，终止时 operand 为空
  ExprResult getPlaceAddress(const Expr *expr,
                           semantic::SemanticContext &semantic_ctx);
  ExprResult generateLiteralExpr(const LiteralExpr *literal_expr,
                                 semantic::SemanticContext &semantic_ctx);
  ExprResult generateUnitExpr(const UnitExpr *unit_expr,
                              semantic::SemanticContext &semantic_ctx);
  ExprResult generatePathExpr(const PathExpr *path_expr,
                              semantic::SemanticContext &semantic_ctx);
  ExprResult generateArrayExpr(const ArrayExpr *array_expr,
                               semantic::SemanticContext &semantic_ctx);
  ExprResult generateStructExpr(const StructExpr *struct_expr,
                                semantic::SemanticContext &semantic_ctx);
  ExprResult generateIfExpr(const IfExpr *if_expr,
                            semantic::SemanticContext &semantic_ctx);
  ExprResult generateLoopExpr(const LoopExpr *loop_expr,
                              semantic::SemanticContext &semantic_ctx);
  ExprResult generateWhileExpr(const WhileExpr *while_expr,
                               semantic::SemanticContext &semantic_ctx);
  ExprResult generateBreakExpr(const BreakExpr *break_expr,
                               semantic::SemanticContext &semantic_ctx);
  ExprResult generateReturnExpr(const ReturnExpr *return_expr,
                                semantic::SemanticContext &semantic_ctx);
  ExprResult generateContinueExpr(const ContinueExpr *continue_expr,
                                  semantic::SemanticContext &semantic_ctx);
  ExprResult generateUnaryExpr(const UnaryExpr *unary_expr,
                               semantic::SemanticContext &semantic_ctx);
  ExprResult generateBinaryExpr(const BinaryExpr *binary_expr,
                                semantic::SemanticContext &semantic_ctx);
  ExprResult generateArithmeticExpr(const BinaryExpr *binary_expr,
                                    semantic::SemanticContext &semantic_ctx);
  ExprResult generateBitwiseExpr(const BinaryExpr *binary_expr,
                                 semantic::SemanticContext &semantic_ctx);
  ExprResult generateShiftExpr(const BinaryExpr *binary_expr,
                               semantic::SemanticContext &semantic_ctx);
  ExprResult generateLogicalExpr(const BinaryExpr *binary_expr,
                                 semantic::SemanticContext &semantic_ctx);
  ExprResult generateEqualityExpr(const BinaryExpr *binary_expr,
                                  semantic::SemanticContext &semantic_ctx);
  ExprResult generateOrderingExpr(const BinaryExpr *binary_expr,
                                  semantic::SemanticContext &semantic_ctx);
  // 比较同一语义类型的两个值，返回 i1
  Operand generateEquality(semantic::TypeId type_id, Operand lhs_operand,
                            Operand rhs_operand,
                            semantic::SemanticContext &semantic_ctx);
  ExprResult generateAssignExpr(const AssignExpr *assign_expr,
                                semantic::SemanticContext &semantic_ctx);
  ExprResult generateCompoundAssignExpr(const AssignExpr *assign_expr,
                                        semantic::SemanticContext &semantic_ctx);
  ExprResult generateCastExpr(const CastExpr *cast_expr,
                              semantic::SemanticContext &semantic_ctx);
  ExprResult generateCallExpr(const CallExpr *call_expr,
                              semantic::SemanticContext &semantic_ctx);
  ExprResult generateIndexExpr(const IndexExpr *index_expr,
                               semantic::SemanticContext &semantic_ctx);
  ExprResult generateMemberExpr(const MemberExpr *member_expr,
                                semantic::SemanticContext &semantic_ctx);
  ExprResult generateMethodCallExpr(const MethodCallExpr *method_call_expr,
                                    semantic::SemanticContext &semantic_ctx);
  Operand applyCoercion(Operand operand, const semantic::ExprInfo &expr_info);
  semantic::TypeId getDereferencedTypeId(
      semantic::TypeId type_id, semantic::DerefKind deref_kind,
      semantic::SemanticContext &semantic_ctx);
  ExprResult generateLetStmt(const LetStmt *let_stmt,
                             semantic::SemanticContext &semantic_ctx);
  ExprResult generateBlockExpr(const BlockExpr *block_expr,
                               semantic::SemanticContext &semantic_ctx);
  void createLocal(semantic::LocalId local_id, Operand initial_operand);
  Value emitAlloca(IRTypeId type_id);
  Value emitLoad(IRTypeId type_id, Value ptr);
  void emitStore(Operand operand, Value ptr);
  Value emitBinary(BinaryOp op, Operand lhs_operand, Operand rhs_operand);
  Value emitCompare(CompareOp op, Operand lhs_operand, Operand rhs_operand);
  Value emitCast(CastOp op, Operand operand, IRTypeId to_type_id);
  Value emitGetElementPtr(IRTypeId type_id, Value ptr,
                          std::vector<Operand> indices);
  Value emitPhi(IRTypeId type_id, std::vector<PhiIncoming> incomings);
  void emitJump(BlockId target_block_id);
  void emitBranch(Operand condition, BlockId true_block_id,
                  BlockId false_block_id);
  void emitReturn(std::optional<Operand> operand = std::nullopt);
  void generateFunctionBody(semantic::SemanticContext &semantic_ctx);
  Value newValue(IRTypeId type_id);
  BlockId newBlock();
  BasicBlock &getBlock(BlockId block_id);
  BasicBlock &getCurrentBlock();
  bool isCurrentBlockTerminated();
  void setInsertPoint(BlockId block_id);
  void emit(Instruction instruction);
  void registerStruct(semantic::SemanticContext &semantic_ctx);
  void fillStructField(semantic::SemanticContext &semantic_ctx);
  void fillFunctionDef(semantic::SemanticContext &semantic_ctx);
  IRTypeId lowerType(semantic::TypeId semantic_type_id,
                     semantic::SemanticContext &semantic_ctx);

  Module module_;
  const Crate *crate_;
  std::unordered_map<semantic::TypeId, IRTypeId>
      semantic_type_id_to_ir_type_id_;
  std::unordered_map<semantic::LocalId, Value> localid_to_place_;

  std::optional<FunctionId> current_function_id_ = std::nullopt;
  std::optional<BlockId> current_block_id_ = std::nullopt;
  std::optional<std::size_t> current_block_index_ = std::nullopt;
};

} // namespace ir
