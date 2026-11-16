#include "IRGenerator.h"
#include <cassert>
#include <stdexcept>
#include <utility>

namespace ir
{

Module IRGenerator::generateIR(semantic::SemanticContext &semantic_ctx)
{
  registerStruct(semantic_ctx);
  fillStructField(semantic_ctx);
  fillFunctionDef(semantic_ctx);
  generateFunctionBody(semantic_ctx);
  return module_;
}

IRTypeId IRGenerator::lowerType(semantic::TypeId semantic_type_id,
                                semantic::SemanticContext &semantic_ctx)
{
  auto it = semantic_type_id_to_ir_type_id_.find(semantic_type_id);
  if (it != semantic_type_id_to_ir_type_id_.end()) // 若有直接返回
  {
    return it->second;
  }
  const semantic::SemType &semantic_type =
      semantic_ctx.getType(semantic_type_id);
  IRTypeId ir_type_id;
  if (dynamic_cast<const semantic::IntegerSemType *>(&semantic_type))
  {
    ir_type_id = module_.ir_type_table_.getOrCreateIntegerType(32);
  }
  else if (dynamic_cast<const semantic::BoolSemType *>(&semantic_type))
  {
    ir_type_id = module_.ir_type_table_.getOrCreateIntegerType(1);
  }
  else if (dynamic_cast<const semantic::UnitSemType *>(&semantic_type))
  {
    ir_type_id = module_.ir_type_table_.getOrCreateVoidType();
  }
  else if (dynamic_cast<const semantic::ReferenceSemType *>(&semantic_type))
  {
    ir_type_id = module_.ir_type_table_.getOrCreatePointerType();
  }
  else if (dynamic_cast<const semantic::BoxSemType *>(&semantic_type))
  {
    ir_type_id = module_.ir_type_table_.getOrCreatePointerType();
  }
  else if (const auto *array_type =
               dynamic_cast<const semantic::ArraySemType *>(&semantic_type))
  {
    IRTypeId element_type_id =
        lowerType(array_type->element_type_id_, semantic_ctx);
    ir_type_id = module_.ir_type_table_.getOrCreateArrayType(
        element_type_id, static_cast<std::size_t>(array_type->length_));
  }
  else if (const auto *vec_type =
               dynamic_cast<const semantic::VecSemType *>(&semantic_type))
  {
    IRTypeId element_type_id =
        lowerType(vec_type->element_type_id_, semantic_ctx);
    ir_type_id = module_.ir_type_table_.getOrCreateVecType(element_type_id);
  }
  semantic_type_id_to_ir_type_id_.emplace(semantic_type_id, ir_type_id);
  return ir_type_id;
}

void IRGenerator::registerStruct(semantic::SemanticContext &semantic_ctx)
{
  for (StructId struct_id = 0; struct_id < semantic_ctx.structs_.size();
       struct_id++)
  {
    const semantic::StructInfo &struct_info = semantic_ctx.structs_[struct_id];
    semantic::TypeId semantic_type_id = struct_info.type_id_.value();
    IRTypeId ir_type_id = module_.ir_type_table_.createStructType(struct_id);
    semantic_type_id_to_ir_type_id_.emplace(semantic_type_id, ir_type_id);
  }
}

void IRGenerator::fillStructField(semantic::SemanticContext &semantic_ctx)
{
  for (StructId struct_id = 0; struct_id < semantic_ctx.structs_.size();
       struct_id++)
  {
    const semantic::StructInfo &struct_info = semantic_ctx.structs_[struct_id];
    Struct ir_struct;
    ir_struct.struct_id_ = struct_id;
    for (auto field_info : struct_info.members_)
    {
      IRTypeId ir_field_type_id = lowerType(field_info.type_id_, semantic_ctx);
      ir_struct.types_.push_back(ir_field_type_id);
    }
    module_.structs_.emplace(struct_id, ir_struct);
  }
}

void IRGenerator::fillFunctionDef(semantic::SemanticContext &semantic_ctx)
{
  for (FunctionId function_id = 0; function_id < semantic_ctx.functions_.size();
       function_id++)
  {
    const semantic::FuncInfo &func_info = semantic_ctx.functions_[function_id];
    Function ir_function;
    ir_function.func_id_ = function_id;
    ir_function.return_type_id_ =
        lowerType(func_info.return_type_id_, semantic_ctx);
    if (func_info.self_param_)
    {
      IRTypeId self_type_id;
      if (func_info.self_param_.value().is_reference_)
      {
        self_type_id = module_.ir_type_table_.getOrCreatePointerType();
      }
      else
      {
        self_type_id =
            module_.ir_type_table_
                .getStructIRTypeId(func_info.owner_struct_id_.value())
                .value();
      }
      ir_function.arg_values_.push_back(newValue(self_type_id));
    }
    for (auto semantic_arg_type_id : func_info.args_type_id_)
    {
      IRTypeId ir_arg_type_id = lowerType(semantic_arg_type_id, semantic_ctx);
      ir_function.arg_values_.push_back(newValue(ir_arg_type_id));
    }
    module_.functions_.emplace(function_id, ir_function);
  }
}

Value IRGenerator::newValue(IRTypeId type_id)
{
  return module_.newValue(type_id);
}

BlockId IRGenerator::newBlock()
{
  assert(current_function_id_.has_value());
  auto &function = module_.functions_.at(current_function_id_.value());
  BlockId block_id = module_.newBlockId();
  function.basic_blocks_.push_back(
      BasicBlock{.block_id_ = block_id, .instructions_ = {}});
  return block_id;
}

BasicBlock &IRGenerator::getBlock(BlockId block_id)
{
  assert(current_function_id_.has_value());
  auto &function = module_.functions_.at(current_function_id_.value());
  for (auto &block : function.basic_blocks_)
  {
    if (block.block_id_ == block_id)
    {
      return block;
    }
  }
}

BasicBlock &IRGenerator::getCurrentBlock()
{
  assert(current_function_id_.has_value());
  assert(current_block_index_.has_value());
  auto &function = module_.functions_.at(current_function_id_.value());
  return function.basic_blocks_.at(current_block_index_.value());
}

bool IRGenerator::isCurrentBlockTerminated()
{
  const auto &block = getCurrentBlock();
  if (block.instructions_.empty())
  {
    return false;
  }
  const Instruction &instruction = block.instructions_.back();
  return std::holds_alternative<JumpInst>(instruction) ||
         std::holds_alternative<BranchInst>(instruction) ||
         std::holds_alternative<ReturnInst>(instruction);
}

void IRGenerator::setInsertPoint(BlockId block_id)
{
  assert(current_function_id_.has_value());
  auto &function = module_.functions_.at(current_function_id_.value());
  for (std::size_t block_index = 0; block_index < function.basic_blocks_.size();
       block_index++)
  {
    if (function.basic_blocks_[block_index].block_id_ == block_id)
    {
      current_block_id_ = block_id;
      current_block_index_ = block_index;
      return;
    }
  }
}

void IRGenerator::emit(Instruction instruction)
{
  assert(!isCurrentBlockTerminated());
  getCurrentBlock().instructions_.push_back(std::move(instruction));
}

ExprResult IRGenerator::generateExpr(const Expr *expr,
                                     semantic::SemanticContext &semantic_ctx)
{
  ExprResult result;
  if (auto literal_expr = dynamic_cast<const LiteralExpr *>(expr))
  {
    result = generateLiteralExpr(literal_expr, semantic_ctx);
  }
  else if (auto unit_expr = dynamic_cast<const UnitExpr *>(expr))
  {
    result = generateUnitExpr(unit_expr, semantic_ctx);
  }
  else if (auto path_expr = dynamic_cast<const PathExpr *>(expr))
  {
    result = generatePathExpr(path_expr, semantic_ctx);
  }
  else if (auto array_expr = dynamic_cast<const ArrayExpr *>(expr))
  {
    result = generateArrayExpr(array_expr, semantic_ctx);
  }
  else if (auto struct_expr = dynamic_cast<const StructExpr *>(expr))
  {
    result = generateStructExpr(struct_expr, semantic_ctx);
  }
  else if (auto block_expr = dynamic_cast<const BlockExpr *>(expr))
  {
    result = generateBlockExpr(block_expr, semantic_ctx);
  }
  else if (auto if_expr = dynamic_cast<const IfExpr *>(expr))
  {
    result = generateIfExpr(if_expr, semantic_ctx);
  }
  else if (auto loop_expr = dynamic_cast<const LoopExpr *>(expr))
  {
    result = generateLoopExpr(loop_expr, semantic_ctx);
  }
  else if (auto while_expr = dynamic_cast<const WhileExpr *>(expr))
  {
    result = generateWhileExpr(while_expr, semantic_ctx);
  }
  else if (auto break_expr = dynamic_cast<const BreakExpr *>(expr))
  {
    result = generateBreakExpr(break_expr, semantic_ctx);
  }
  else if (auto return_expr = dynamic_cast<const ReturnExpr *>(expr))
  {
    result = generateReturnExpr(return_expr, semantic_ctx);
  }
  else if (auto continue_expr = dynamic_cast<const ContinueExpr *>(expr))
  {
    result = generateContinueExpr(continue_expr, semantic_ctx);
  }
  else if (auto unary_expr = dynamic_cast<const UnaryExpr *>(expr))
  {
    result = generateUnaryExpr(unary_expr, semantic_ctx);
  }
  else if (auto binary_expr = dynamic_cast<const BinaryExpr *>(expr))
  {
    result = generateBinaryExpr(binary_expr, semantic_ctx);
  }
  else if (auto assign_expr = dynamic_cast<const AssignExpr *>(expr))
  {
    result = generateAssignExpr(assign_expr, semantic_ctx);
  }
  else if (auto cast_expr = dynamic_cast<const CastExpr *>(expr))
  {
    result = generateCastExpr(cast_expr, semantic_ctx);
  }
  else if (auto call_expr = dynamic_cast<const CallExpr *>(expr))
  {
    result = generateCallExpr(call_expr, semantic_ctx);
  }
  else if (auto index_expr = dynamic_cast<const IndexExpr *>(expr))
  {
    result = generateIndexExpr(index_expr, semantic_ctx);
  }
  else if (auto member_expr = dynamic_cast<const MemberExpr *>(expr))
  {
    result = generateMemberExpr(member_expr, semantic_ctx);
  }
  else if (auto method_call_expr = dynamic_cast<const MethodCallExpr *>(expr))
  {
    result = generateMethodCallExpr(method_call_expr, semantic_ctx);
  }
  if (result.terminated_)
  {
    return result;
  }
  // 处理隐式转换
  if (result.operand_.has_value())
  {
    const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(expr);
    result.operand_ = applyCoercion(result.operand_.value(), expr_info);
  }
  return result;
}

ExprResult IRGenerator::getPlaceAddress(const Expr *expr,
                                        semantic::SemanticContext &semantic_ctx)
{
  const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(expr);
  // x
  if (auto path_expr = dynamic_cast<const PathExpr *>(expr))
  {
    if (expr_info.local_id_.has_value())
    {
      return ExprResult{.terminated_ = false,
                        .operand_ =
                            localid_to_place_.at(expr_info.local_id_.value())};
    }
  }
  // *p
  if (auto unary_expr = dynamic_cast<const UnaryExpr *>(expr))
  {
    if (unary_expr->op_ == UnaryOperator::Dereference)
    {
      return generateExpr(unary_expr->expr_.get(), semantic_ctx);
    }
  }
  // s.field 和 a[i]
  const auto *member_expr = dynamic_cast<const MemberExpr *>(expr);
  const auto *index_expr = dynamic_cast<const IndexExpr *>(expr);
  if (member_expr || index_expr)
  {
    const Expr *base;
    if (member_expr)
    {
      base = member_expr->base_.get();
    }
    else
    {
      base = index_expr->base_.get();
    }
    ExprResult base_result;
    if (expr_info.base_derefs_.empty())
    {
      base_result = getPlaceAddress(base, semantic_ctx);
    }
    else
    {
      base_result = generateExpr(base, semantic_ctx);
    }
    // base 终止
    if (base_result.terminated_)
    {
      return base_result;
    }
    Value base_ptr = *std::get_if<Value>(&base_result.operand_.value());
    semantic::TypeId base_type_id =
        semantic_ctx.expr_to_exprinfos_.at(base).type_id_;
    IRTypeId ptr_type_id = module_.ir_type_table_.getOrCreatePointerType();
    // 第一层地址已由 generateExpr 得到，只更新类型
    if (!expr_info.base_derefs_.empty())
    {
      base_type_id = getDereferencedTypeId(
          base_type_id, expr_info.base_derefs_[0], semantic_ctx);
      // 后续每层读取指针，并更新类型
      for (std::size_t i = 1; i < expr_info.base_derefs_.size(); i++)
      {
        // load出的是一个ptr
        base_ptr = emitLoad(ptr_type_id, base_ptr);
        base_type_id = getDereferencedTypeId(
            base_type_id, expr_info.base_derefs_[i], semantic_ctx);
      }
    }
    // 最后base_type_id为struct或array类型
    IRTypeId index_type_id =
        module_.ir_type_table_.getOrCreateIntegerType(32); // i32
    std::vector<Operand> indices;
    indices.push_back(IntegerConst{
        .value_ = 0,
        .type_id_ = index_type_id}); // 第一个以struct或array为单位，不动
    // member字段号已记录
    if (member_expr)
    {
      indices.push_back(IntegerConst{.value_ = expr_info.member_id_.value(),
                                     .type_id_ = index_type_id});
    }
    // index需计算
    else
    {
      ExprResult index_result =
          generateExpr(index_expr->index_.get(), semantic_ctx);
      if (index_result.terminated_)
      {
        return index_result;
      }
      if (dynamic_cast<const semantic::VecSemType *>(
              &semantic_ctx.getType(base_type_id)))
      {
        throw std::logic_error("Vec element addresses are not implemented");
      }
      indices.push_back(index_result.operand_.value());
    }
    Value result = emitGetElementPtr(lowerType(base_type_id, semantic_ctx),
                                     base_ptr, std::move(indices));
    return ExprResult{.terminated_ = false, .operand_ = result};
  }
  // 值表达式临时物化一个地址
  ExprResult result = generateExpr(expr, semantic_ctx);
  if (result.terminated_)
  {
    return result;
  }
  Operand operand = result.operand_.value();
  Value addr = emitAlloca(getOperandTypeId(operand));
  emitStore(operand, addr);
  return ExprResult{.terminated_ = false, .operand_ = addr};
}

ExprResult
IRGenerator::generateLiteralExpr(const LiteralExpr *literal_expr,
                                 semantic::SemanticContext &semantic_ctx)
{
  const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(literal_expr);
  IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
  std::uint64_t value;
  if (literal_expr->is_int_)
  {
    value = literal_expr->int_value_;
  }
  else
  {
    value = literal_expr->bool_value_ ? 1 : 0;
  }
  return ExprResult{.terminated_ = false,
                    .operand_ =
                        IntegerConst{.value_ = value, .type_id_ = type_id}};
}

ExprResult
IRGenerator::generateUnitExpr(const UnitExpr *unit_expr,
                              semantic::SemanticContext &semantic_ctx)
{
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generatePathExpr(const PathExpr *path_expr,
                              semantic::SemanticContext &semantic_ctx)
{
  const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(path_expr);
  IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
  if (expr_info.local_id_.has_value())
  {
    Value place = localid_to_place_.at(expr_info.local_id_.value());
    return ExprResult{.terminated_ = false,
                      .operand_ = emitLoad(type_id, place)};
  }
  if (expr_info.const_id_.has_value())
  {
    const auto &const_info =
        semantic_ctx.constants_[expr_info.const_id_.value()];
    const auto &const_value = const_info.value_.value();
    std::uint64_t value;
    if (auto int_value = std::get_if<std::int64_t>(&const_value))
    {
      value = static_cast<std::uint64_t>(*int_value);
    }
    else
    {
      value = std::get<bool>(const_value) ? 1 : 0;
    }
    return ExprResult{.terminated_ = false,
                      .operand_ =
                          IntegerConst{.value_ = value, .type_id_ = type_id}};
  }
}

ExprResult
IRGenerator::generateBinaryExpr(const BinaryExpr *binary_expr,
                                semantic::SemanticContext &semantic_ctx)
{
  if (binary_expr->op_ == BinaryOperator::LogicalAnd ||
      binary_expr->op_ == BinaryOperator::LogicalOr)
  {
    return generateLogicalExpr(binary_expr, semantic_ctx);
  }
  else if (binary_expr->op_ == BinaryOperator::Equal ||
           binary_expr->op_ == BinaryOperator::NotEqual)
  {
    return generateEqualityExpr(binary_expr, semantic_ctx);
  }
  else if (binary_expr->op_ == BinaryOperator::Less ||
           binary_expr->op_ == BinaryOperator::LessEqual ||
           binary_expr->op_ == BinaryOperator::Greater ||
           binary_expr->op_ == BinaryOperator::GreaterEqual)
  {
    return generateOrderingExpr(binary_expr, semantic_ctx);
  }
  else if (binary_expr->op_ == BinaryOperator::Add ||
           binary_expr->op_ == BinaryOperator::Subtract ||
           binary_expr->op_ == BinaryOperator::Multiply ||
           binary_expr->op_ == BinaryOperator::Divide ||
           binary_expr->op_ == BinaryOperator::Remainder)
  {
    return generateArithmeticExpr(binary_expr, semantic_ctx);
  }
  else if (binary_expr->op_ == BinaryOperator::BitAnd ||
           binary_expr->op_ == BinaryOperator::BitOr ||
           binary_expr->op_ == BinaryOperator::BitXor)
  {
    return generateBitwiseExpr(binary_expr, semantic_ctx);
  }
  else if (binary_expr->op_ == BinaryOperator::ShiftLeft ||
           binary_expr->op_ == BinaryOperator::ShiftRight)
  {
    return generateShiftExpr(binary_expr, semantic_ctx);
  }
  else
  {
    throw std::logic_error("Unsupported binary operator");
  }
}

ExprResult
IRGenerator::generateShiftExpr(const BinaryExpr *binary_expr,
                               semantic::SemanticContext &semantic_ctx)
{
  ExprResult lhs_result = generateExpr(binary_expr->lhs_.get(), semantic_ctx);
  if (lhs_result.terminated_)
  {
    return lhs_result;
  }
  ExprResult rhs_result = generateExpr(binary_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  Operand lhs_operand = lhs_result.operand_.value();
  Operand rhs_operand = rhs_result.operand_.value();
  const auto &lhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->lhs_.get());
  const auto &rhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->rhs_.get());
  semantic::TypeId lhs_type_id = lhs_info.type_id_;
  semantic::TypeId rhs_type_id = rhs_info.type_id_;
  // 解引用
  while (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
             &semantic_ctx.getType(lhs_type_id)))
  {
    lhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&lhs_operand);
    lhs_operand = emitLoad(lowerType(lhs_type_id, semantic_ctx), *ptr);
  }
  while (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
             &semantic_ctx.getType(rhs_type_id)))
  {
    rhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&rhs_operand);
    rhs_operand = emitLoad(lowerType(rhs_type_id, semantic_ctx), *ptr);
  }
  bool is_signed = false;
  const auto &scalar_type = semantic_ctx.getType(lhs_type_id);
  // 是否有符号由lhs决定
  if (auto integer_type =
          dynamic_cast<const semantic::IntegerSemType *>(&scalar_type))
  {
    is_signed = integer_type->integer_type_ == IntegerType::I32 ||
                integer_type->integer_type_ == IntegerType::ISize;
  }
  BinaryOp op;
  // 移位运算
  rhs_operand = emitBinary(
      BinaryOp::BitAnd, rhs_operand,
      IntegerConst{.value_ = 31, .type_id_ = getOperandTypeId(rhs_operand)});
  if (binary_expr->op_ == BinaryOperator::ShiftLeft)
  {
    op = BinaryOp::ShiftLeft;
  }
  else if (is_signed)
  {
    op = BinaryOp::ArithmeticShiftRight;
  }
  else
  {
    op = BinaryOp::LogicalShiftRight;
  }
  return ExprResult{.terminated_ = false,
                    .operand_ = emitBinary(op, lhs_operand, rhs_operand)};
}

ExprResult
IRGenerator::generateBitwiseExpr(const BinaryExpr *binary_expr,
                                 semantic::SemanticContext &semantic_ctx)
{
  ExprResult lhs_result = generateExpr(binary_expr->lhs_.get(), semantic_ctx);
  if (lhs_result.terminated_)
  {
    return lhs_result;
  }
  ExprResult rhs_result = generateExpr(binary_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  Operand lhs_operand = lhs_result.operand_.value();
  Operand rhs_operand = rhs_result.operand_.value();
  const auto &lhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->lhs_.get());
  const auto &rhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->rhs_.get());
  semantic::TypeId lhs_type_id = lhs_info.type_id_;
  semantic::TypeId rhs_type_id = rhs_info.type_id_;
  // &T
  if (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
          &semantic_ctx.getType(lhs_type_id)))
  {
    lhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&lhs_operand);
    lhs_operand = emitLoad(lowerType(lhs_type_id, semantic_ctx), *ptr);
  }
  if (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
          &semantic_ctx.getType(rhs_type_id)))
  {
    rhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&rhs_operand);
    rhs_operand = emitLoad(lowerType(rhs_type_id, semantic_ctx), *ptr);
  }
  BinaryOp op;
  // 按位运算
  if (binary_expr->op_ == BinaryOperator::BitAnd)
  {
    op = BinaryOp::BitAnd;
  }
  else if (binary_expr->op_ == BinaryOperator::BitOr)
  {
    op = BinaryOp::BitOr;
  }
  else
  {
    op = BinaryOp::BitXor;
  }
  return ExprResult{.terminated_ = false,
                    .operand_ = emitBinary(op, lhs_operand, rhs_operand)};
}

ExprResult
IRGenerator::generateArithmeticExpr(const BinaryExpr *binary_expr,
                                    semantic::SemanticContext &semantic_ctx)
{
  ExprResult lhs_result = generateExpr(binary_expr->lhs_.get(), semantic_ctx);
  if (lhs_result.terminated_)
  {
    return lhs_result;
  }
  ExprResult rhs_result = generateExpr(binary_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  Operand lhs_operand = lhs_result.operand_.value();
  Operand rhs_operand = rhs_result.operand_.value();
  const auto &lhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->lhs_.get());
  const auto &rhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->rhs_.get());
  semantic::TypeId lhs_type_id = lhs_info.type_id_;
  semantic::TypeId rhs_type_id = rhs_info.type_id_;
  // &T
  if (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
          &semantic_ctx.getType(lhs_type_id)))
  {
    lhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&lhs_operand);
    lhs_operand = emitLoad(lowerType(lhs_type_id, semantic_ctx), *ptr);
  }
  if (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
          &semantic_ctx.getType(rhs_type_id)))
  {
    rhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&rhs_operand);
    rhs_operand = emitLoad(lowerType(rhs_type_id, semantic_ctx), *ptr);
  }
  bool is_signed = false;
  const auto &scalar_type = semantic_ctx.getType(lhs_type_id);
  // 是否有符号由lhs决定，由于左右类型一样
  if (auto integer_type =
          dynamic_cast<const semantic::IntegerSemType *>(&scalar_type))
  {
    is_signed = integer_type->integer_type_ == IntegerType::I32 ||
                integer_type->integer_type_ == IntegerType::ISize;
  }
  BinaryOp op;
  // 算术运算
  if (binary_expr->op_ == BinaryOperator::Add)
  {
    op = BinaryOp::Add;
  }
  else if (binary_expr->op_ == BinaryOperator::Subtract)
  {
    op = BinaryOp::Sub;
  }
  else if (binary_expr->op_ == BinaryOperator::Multiply)
  {
    op = BinaryOp::Mul;
  }
  else if (binary_expr->op_ == BinaryOperator::Divide)
  {
    if (is_signed)
    {
      op = BinaryOp::SignedDiv;
    }
    else
    {
      op = BinaryOp::UnsignedDiv;
    }
  }
  else
  {
    if (is_signed)
    {
      op = BinaryOp::SignedRem;
    }
    else
    {
      op = BinaryOp::UnsignedRem;
    }
  }
  return ExprResult{.terminated_ = false,
                    .operand_ = emitBinary(op, lhs_operand, rhs_operand)};
}

ExprResult
IRGenerator::generateOrderingExpr(const BinaryExpr *binary_expr,
                                  semantic::SemanticContext &semantic_ctx)
{
  ExprResult lhs_result = generateExpr(binary_expr->lhs_.get(), semantic_ctx);
  if (lhs_result.terminated_)
  {
    return lhs_result;
  }
  ExprResult rhs_result = generateExpr(binary_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  Operand lhs_operand = lhs_result.operand_.value();
  Operand rhs_operand = rhs_result.operand_.value();
  const auto &lhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->lhs_.get());
  const auto &rhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->rhs_.get());
  semantic::TypeId lhs_type_id = lhs_info.type_id_;
  semantic::TypeId rhs_type_id = rhs_info.type_id_;
  // 解引用
  while (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
             &semantic_ctx.getType(lhs_type_id)))
  {
    lhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&lhs_operand);
    lhs_operand = emitLoad(lowerType(lhs_type_id, semantic_ctx), *ptr);
  }
  while (auto reference_type = dynamic_cast<const semantic::ReferenceSemType *>(
             &semantic_ctx.getType(rhs_type_id)))
  {
    rhs_type_id = reference_type->inner_type_id_;
    auto ptr = std::get_if<Value>(&rhs_operand);
    rhs_operand = emitLoad(lowerType(rhs_type_id, semantic_ctx), *ptr);
  }
  bool is_signed = false;
  const auto &scalar_type = semantic_ctx.getType(lhs_type_id);
  // 是否有符号由lhs决定
  if (auto integer_type =
          dynamic_cast<const semantic::IntegerSemType *>(&scalar_type))
  {
    is_signed = integer_type->integer_type_ == IntegerType::I32 ||
                integer_type->integer_type_ == IntegerType::ISize;
  }
  CompareOp compare_op;
  // 大小比较，bool 使用无符号比较，false 小于 true
  if (binary_expr->op_ == BinaryOperator::Less)
  {
    if (is_signed)
    {
      compare_op = CompareOp::SignedLess;
    }
    else
    {
      compare_op = CompareOp::UnsignedLess;
    }
  }
  else if (binary_expr->op_ == BinaryOperator::LessEqual)
  {
    if (is_signed)
    {
      compare_op = CompareOp::SignedLessEqual;
    }
    else
    {
      compare_op = CompareOp::UnsignedLessEqual;
    }
  }
  else if (binary_expr->op_ == BinaryOperator::Greater)
  {
    if (is_signed)
    {
      compare_op = CompareOp::SignedGreater;
    }
    else
    {
      compare_op = CompareOp::UnsignedGreater;
    }
  }
  else
  {
    if (is_signed)
    {
      compare_op = CompareOp::SignedGreaterEqual;
    }
    else
    {
      compare_op = CompareOp::UnsignedGreaterEqual;
    }
  }
  return ExprResult{.terminated_ = false,
                    .operand_ =
                        emitCompare(compare_op, lhs_operand, rhs_operand)};
}

ExprResult IRGenerator::generateContinueExpr(
    const ContinueExpr *continue_expr, semantic::SemanticContext &semantic_ctx)
{
  emitJump(loop_stack_.back().continue_block_id_);
  return ExprResult{.terminated_ = true, .operand_ = std::nullopt};
}

ExprResult IRGenerator::generateBreakExpr(
    const BreakExpr *break_expr, semantic::SemanticContext &semantic_ctx)
{
  ExprResult value_result;
  if (break_expr->value_)
  {
    value_result = generateExpr(break_expr->value_.get(), semantic_ctx);
    if (value_result.terminated_)
    {
      return value_result;
    }
  }
  // 操作数可能包含嵌套循环，生成完后再取得当前循环上下文
  auto &context = loop_stack_.back();
  if (!context.break_block_id_.has_value())
  {
    // loop中第一次能跳出去，创建loop_out
    context.break_block_id_ = newBlock();
  }
  if (value_result.operand_.has_value())
  {
    // 有返回值，且真能break出去
    context.break_incomings_.push_back(
        PhiIncoming{.block_id_ = getCurrentBlock().block_id_,
                     .operand_ = value_result.operand_.value()});
  }
  // jump到loop_out
  emitJump(context.break_block_id_.value());
  return ExprResult{.terminated_ = true, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generateLoopExpr(const LoopExpr *loop_expr,
                              semantic::SemanticContext &semantic_ctx)
{
  BlockId loop_body_block_id = newBlock();
  emitJump(loop_body_block_id);
  loop_stack_.push_back(LoopContext{.continue_block_id_ = loop_body_block_id,
                                    .break_block_id_ = std::nullopt,
                                    .break_incomings_ = {}});
  setInsertPoint(loop_body_block_id);
  ExprResult body_result =
      generateExpr(loop_expr->block_expr_.get(), semantic_ctx);
  if (!body_result.terminated_)
  {
    emitJump(loop_body_block_id);
  }
  LoopContext context = std::move(loop_stack_.back());
  loop_stack_.pop_back();
  if (!context.break_block_id_.has_value())
  {
    // 没创建loop_out，说明根本没跳出来过，返回终止
    return ExprResult{.terminated_ = true, .operand_ = std::nullopt};
  }
  setInsertPoint(context.break_block_id_.value());
  if (context.break_incomings_.empty())
  {
    // 返回都是unit
    return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
  }
  if (context.break_incomings_.size() == 1)
  {
    // 一个break不用创建phi
    return ExprResult{.terminated_ = false,
                      .operand_ = context.break_incomings_[0].operand_};
  }
  IRTypeId type_id = getOperandTypeId(context.break_incomings_[0].operand_);
  return ExprResult{.terminated_ = false,
                    .operand_ =
                        emitPhi(type_id, std::move(context.break_incomings_))};
}

ExprResult
IRGenerator::generateWhileExpr(const WhileExpr *while_expr,
                               semantic::SemanticContext &semantic_ctx)
{
  BlockId condition_block_id = newBlock();
  emitJump(condition_block_id);
  setInsertPoint(condition_block_id);
  ExprResult condition_result =
      generateExpr(while_expr->condition_expr_.get(), semantic_ctx);
  if (condition_result.terminated_)
  {
    return condition_result;
  }
  BlockId body_block_id = newBlock();
  BlockId out_block_id = newBlock();
  emitBranch(condition_result.operand_.value(), body_block_id, out_block_id);
  loop_stack_.push_back(LoopContext{.continue_block_id_ = condition_block_id,
                                    .break_block_id_ = out_block_id,
                                    .break_incomings_ = {}});
  setInsertPoint(body_block_id);
  ExprResult body_result =
      generateExpr(while_expr->block_expr_.get(), semantic_ctx);
  if (!body_result.terminated_)
  {
    emitJump(condition_block_id);
  }
  loop_stack_.pop_back();
  // 只有block终止没事，可以直接跳到out块，while返回unit
  setInsertPoint(out_block_id);
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

ExprResult IRGenerator::generateIfExpr(const IfExpr *if_expr,
                                       semantic::SemanticContext &semantic_ctx)
{
  ExprResult condition_result =
      generateExpr(if_expr->condition_expr_.get(), semantic_ctx);
  if (condition_result.terminated_)
  {
    return condition_result;
  }
  if (if_expr->has_else_)
  {
    BlockId then_block_id = newBlock();
    BlockId else_block_id = newBlock();
    emitBranch(condition_result.operand_.value(), then_block_id, else_block_id);
    // then
    setInsertPoint(then_block_id);
    ExprResult then_result =
        generateExpr(if_expr->block_expr_.get(), semantic_ctx);
    BlockId then_end_block_id = getCurrentBlock().block_id_;
    // else or else if
    setInsertPoint(else_block_id);
    ExprResult else_result;
    if (if_expr->is_else_if_)
    {
      else_result = generateExpr(if_expr->else_if_expr_.get(), semantic_ctx);
    }
    else
    {
      else_result = generateExpr(if_expr->else_block_expr_.get(), semantic_ctx);
    }
    BlockId else_end_block_id = getCurrentBlock().block_id_;
    // 都终止则不生成 merge
    if (then_result.terminated_ && else_result.terminated_)
    {
      return ExprResult{.terminated_ = true, .operand_ = std::nullopt};
    }
    // 至少一侧正常结束时，才创建合并块
    BlockId merge_block_id = newBlock();
    if (!then_result.terminated_)
    {
      setInsertPoint(then_end_block_id);
      emitJump(merge_block_id);
    }
    if (!else_result.terminated_)
    {
      setInsertPoint(else_end_block_id);
      emitJump(merge_block_id);
    }
    setInsertPoint(merge_block_id);
    // 只有一侧正常结束，直接转发该侧结果
    if (then_result.terminated_)
    {
      return else_result;
    }
    if (else_result.terminated_)
    {
      return then_result;
    }
    // 若block有值
    if (then_result.operand_.has_value())
    {
      std::vector<PhiIncoming> incomings;
      incomings.push_back(
          PhiIncoming{.block_id_ = then_end_block_id,
                      .operand_ = then_result.operand_.value()});
      incomings.push_back(
          PhiIncoming{.block_id_ = else_end_block_id,
                      .operand_ = else_result.operand_.value()});
      IRTypeId type_id = getOperandTypeId(then_result.operand_.value());
      return ExprResult{.terminated_ = false,
                        .operand_ = emitPhi(type_id, std::move(incomings))};
    }
    // 无值则返回 unit
    return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
  }
  // 无 else
  BlockId then_block_id = newBlock();
  BlockId merge_block_id = newBlock();
  emitBranch(condition_result.operand_.value(), then_block_id, merge_block_id);
  setInsertPoint(then_block_id);
  ExprResult then_result =
      generateExpr(if_expr->block_expr_.get(), semantic_ctx);
  // then 不终止需要最后 jump 回 merge
  if (!then_result.terminated_)
  {
    emitJump(merge_block_id);
  }
  // 最后返回merge块，无else时结果为unit
  setInsertPoint(merge_block_id);
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generateLogicalExpr(const BinaryExpr *binary_expr,
                                 semantic::SemanticContext &semantic_ctx)
{
  ExprResult lhs_result = generateExpr(binary_expr->lhs_.get(), semantic_ctx);
  if (lhs_result.terminated_)
  {
    return lhs_result;
  }
  IRTypeId bool_type_id = module_.ir_type_table_.getOrCreateIntegerType(1);
  BlockId lhs_end_block_id = getCurrentBlock().block_id_;
  BlockId rhs_block_id = newBlock();
  BlockId merge_block_id = newBlock();
  std::uint64_t short_value;
  if (binary_expr->op_ == BinaryOperator::LogicalAnd)
  {
    short_value = 0;
    emitBranch(lhs_result.operand_.value(), rhs_block_id, merge_block_id);
  }
  else
  {
    short_value = 1;
    emitBranch(lhs_result.operand_.value(), merge_block_id, rhs_block_id);
  }
  Operand short_operand =
      IntegerConst{.value_ = short_value, .type_id_ = bool_type_id};
  // rhs block
  setInsertPoint(rhs_block_id);
  ExprResult rhs_result = generateExpr(binary_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    // 只有一边终止，不算总体终止，然后之后继续写merge块
    setInsertPoint(merge_block_id);
    return ExprResult{.terminated_ = false, .operand_ = short_operand};
  }
  // 可能会出现右侧生成了很多个block，需要知道现在在哪个block
  BlockId rhs_end_block_id = getCurrentBlock().block_id_;
  emitJump(merge_block_id);
  setInsertPoint(merge_block_id);
  std::vector<PhiIncoming> incomings;
  incomings.push_back(
      PhiIncoming{.block_id_ = lhs_end_block_id, .operand_ = short_operand});
  incomings.push_back(PhiIncoming{.block_id_ = rhs_end_block_id,
                                  .operand_ = rhs_result.operand_.value()});
  return ExprResult{.terminated_ = false,
                    .operand_ = emitPhi(bool_type_id, std::move(incomings))};
}

ExprResult
IRGenerator::generateEqualityExpr(const BinaryExpr *binary_expr,
                                  semantic::SemanticContext &semantic_ctx)
{
  ExprResult lhs_result = generateExpr(binary_expr->lhs_.get(), semantic_ctx);
  if (lhs_result.terminated_)
  {
    return lhs_result;
  }
  ExprResult rhs_result = generateExpr(binary_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  Operand lhs_operand = lhs_result.operand_.value();
  Operand rhs_operand = rhs_result.operand_.value();
  const auto &lhs_info =
      semantic_ctx.expr_to_exprinfos_.at(binary_expr->lhs_.get());
  Operand result = generateEquality(lhs_info.type_id_, lhs_operand, rhs_operand,
                                    semantic_ctx);
  if (binary_expr->op_ == BinaryOperator::NotEqual)
  {
    // != 结果取反
    result = emitBinary(
        BinaryOp::BitXor, result,
        IntegerConst{.value_ = 1, .type_id_ = getOperandTypeId(result)});
  }
  return ExprResult{.terminated_ = false, .operand_ = result};
}

ExprResult
IRGenerator::generateAssignExpr(const AssignExpr *assign_expr,
                                semantic::SemanticContext &semantic_ctx)
{
  if (assign_expr->op_ != AssignmentOperator::Assign)
  {
    // 复合赋值
    return generateCompoundAssignExpr(assign_expr, semantic_ctx);
  }
  // 普通赋值 =
  ExprResult rhs_result = generateExpr(assign_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  ExprResult place_result =
      getPlaceAddress(assign_expr->lhs_.get(), semantic_ctx);
  if (place_result.terminated_)
  {
    return place_result;
  }
  auto rhs_ptr = std::get_if<Value>(&place_result.operand_.value());
  emitStore(rhs_result.operand_.value(), *rhs_ptr);
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generateCompoundAssignExpr(const AssignExpr *assign_expr,
                                        semantic::SemanticContext &semantic_ctx)
{
  // 求rhs与lhs_place
  ExprResult rhs_result = generateExpr(assign_expr->rhs_.get(), semantic_ctx);
  if (rhs_result.terminated_)
  {
    return rhs_result;
  }
  ExprResult place_result =
      getPlaceAddress(assign_expr->lhs_.get(), semantic_ctx);
  if (place_result.terminated_)
  {
    return place_result;
  }
  auto rhs_ptr = std::get_if<Value>(&place_result.operand_.value());
  Operand rhs_operand = rhs_result.operand_.value();
  const auto &lhs_info =
      semantic_ctx.expr_to_exprinfos_.at(assign_expr->lhs_.get());
  IRTypeId type_id = lowerType(lhs_info.type_id_, semantic_ctx);
  Operand lhs_operand = emitLoad(type_id, *rhs_ptr);
  const auto &rhs_info =
      semantic_ctx.expr_to_exprinfos_.at(assign_expr->rhs_.get());
  const auto &rhs_type = semantic_ctx.getType(rhs_info.type_id_);
  if (auto reference_type =
          dynamic_cast<const semantic::ReferenceSemType *>(&rhs_type))
  {
    // 若为&T，load 出 T
    auto rhs_ptr = std::get_if<Value>(&rhs_operand);
    rhs_operand = emitLoad(
        lowerType(reference_type->inner_type_id_, semantic_ctx), *rhs_ptr);
  }
  // 由语义type决定是否是有符号运算
  bool is_signed = false;
  const auto &lhs_type = semantic_ctx.getType(lhs_info.type_id_);
  if (auto integer_type =
          dynamic_cast<const semantic::IntegerSemType *>(&lhs_type))
  {
    is_signed = integer_type->integer_type_ == IntegerType::I32 ||
                integer_type->integer_type_ == IntegerType::ISize;
  }
  BinaryOp op;
  if (assign_expr->op_ == AssignmentOperator::AddAssign)
  {
    op = BinaryOp::Add;
  }
  else if (assign_expr->op_ == AssignmentOperator::SubtractAssign)
  {
    op = BinaryOp::Sub;
  }
  else if (assign_expr->op_ == AssignmentOperator::MultiplyAssign)
  {
    op = BinaryOp::Mul;
  }
  else if (assign_expr->op_ == AssignmentOperator::DivideAssign)
  {
    if (is_signed)
    {
      op = BinaryOp::SignedDiv;
    }
    else
    {
      op = BinaryOp::UnsignedDiv;
    }
  }
  else if (assign_expr->op_ == AssignmentOperator::RemainderAssign)
  {
    if (is_signed)
    {
      op = BinaryOp::SignedRem;
    }
    else
    {
      op = BinaryOp::UnsignedRem;
    }
  }
  else if (assign_expr->op_ == AssignmentOperator::BitAndAssign)
  {
    op = BinaryOp::BitAnd;
  }
  else if (assign_expr->op_ == AssignmentOperator::BitOrAssign)
  {
    op = BinaryOp::BitOr;
  }
  else if (assign_expr->op_ == AssignmentOperator::BitXorAssign)
  {
    op = BinaryOp::BitXor;
  }
  else
  {
    // 移位量只保留低 5 位，但还是 i32
    rhs_operand = emitBinary(
        BinaryOp::BitAnd, rhs_operand,
        IntegerConst{.value_ = 31, .type_id_ = getOperandTypeId(rhs_operand)});
    if (assign_expr->op_ == AssignmentOperator::ShiftLeftAssign)
    {
      op = BinaryOp::ShiftLeft;
    }
    else if (is_signed)
    {
      op = BinaryOp::ArithmeticShiftRight;
    }
    else
    {
      op = BinaryOp::LogicalShiftRight;
    }
  }
  Value result = emitBinary(op, lhs_operand, rhs_operand);
  emitStore(result, *rhs_ptr);
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generateCastExpr(const CastExpr *cast_expr,
                              semantic::SemanticContext &semantic_ctx)
{
  ExprResult result = generateExpr(cast_expr->expr_.get(), semantic_ctx);
  if (result.terminated_)
  {
    return result;
  }
  const auto &operand_info =
      semantic_ctx.expr_to_exprinfos_.at(cast_expr->expr_.get());
  const auto &operand_type = semantic_ctx.getType(operand_info.type_id_);
  if (dynamic_cast<const semantic::BoolSemType *>(&operand_type))
  {
    const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(cast_expr);
    IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
    result.operand_ =
        emitCast(CastOp::ZeroExt, result.operand_.value(), type_id);
  }
  // 所有整数均为 i32，整数之间转换不用生成实际行
  return result;
}

ExprResult
IRGenerator::generateIndexExpr(const IndexExpr *index_expr,
                               semantic::SemanticContext &semantic_ctx)
{
  ExprResult place_result = getPlaceAddress(index_expr, semantic_ctx);
  if (place_result.terminated_)
  {
    return place_result;
  }
  const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(index_expr);
  IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
  auto ptr = std::get_if<Value>(&place_result.operand_.value());
  return ExprResult{.terminated_ = false, .operand_ = emitLoad(type_id, *ptr)};
}

ExprResult
IRGenerator::generateMemberExpr(const MemberExpr *member_expr,
                                semantic::SemanticContext &semantic_ctx)
{
  ExprResult place_result = getPlaceAddress(member_expr, semantic_ctx);
  if (place_result.terminated_)
  {
    return place_result;
  }
  // load 出的 type
  const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(member_expr);
  IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
  auto ptr = std::get_if<Value>(&place_result.operand_.value());
  return ExprResult{.terminated_ = false, .operand_ = emitLoad(type_id, *ptr)};
}

ExprResult
IRGenerator::generateReturnExpr(const ReturnExpr *return_expr,
                                semantic::SemanticContext &semantic_ctx)
{
  if (return_expr->value_)
  {
    ExprResult result = generateExpr(return_expr->value_.get(), semantic_ctx);
    if (result.terminated_)
    {
      return result;
    }
    emitReturn(result.operand_);
  }
  else
  {
    emitReturn();
  }
  return ExprResult{.terminated_ = true, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generateUnaryExpr(const UnaryExpr *unary_expr,
                               semantic::SemanticContext &semantic_ctx)
{
  if (unary_expr->op_ == UnaryOperator::Borrow ||
      unary_expr->op_ == UnaryOperator::BorrowMut)
  {
    return getPlaceAddress(unary_expr->expr_.get(), semantic_ctx);
  }
  if (unary_expr->op_ == UnaryOperator::Dereference)
  {
    ExprResult place_result = getPlaceAddress(unary_expr, semantic_ctx);
    if (place_result.terminated_)
    {
      return place_result;
    }
    const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(unary_expr);
    IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
    auto ptr = std::get_if<Value>(&place_result.operand_.value());
    return ExprResult{.terminated_ = false,
                      .operand_ = emitLoad(type_id, *ptr)};
  }
  // -x 和 &x
  ExprResult operand_result =
      generateExpr(unary_expr->expr_.get(), semantic_ctx);
  if (operand_result.terminated_)
  {
    return operand_result;
  }
  Operand operand = operand_result.operand_.value();
  const auto &expr_info = semantic_ctx.expr_to_exprinfos_.at(unary_expr);
  IRTypeId type_id = lowerType(expr_info.type_id_, semantic_ctx);
  const auto &operand_info =
      semantic_ctx.expr_to_exprinfos_.at(unary_expr->expr_.get());
  const auto &operand_type = semantic_ctx.getType(operand_info.type_id_);
  if (dynamic_cast<const semantic::ReferenceSemType *>(&operand_type))
  {
    // &x, 此时operand为x_addr，需要load出x
    auto ptr = std::get_if<Value>(&operand);
    operand = emitLoad(type_id, *ptr);
  }
  Value result;
  if (unary_expr->op_ == UnaryOperator::Negate)
  {
    result = emitBinary(
        BinaryOp::Sub, IntegerConst{.value_ = 0, .type_id_ = type_id}, operand);
  }
  else
  {
    std::uint64_t mask = 0xffffffff;
    if (dynamic_cast<const semantic::BoolSemType *>(
            &semantic_ctx.getType(expr_info.type_id_)))
    {
      mask = 1;
    }
    result = emitBinary(BinaryOp::BitXor, operand,
                        IntegerConst{.value_ = mask, .type_id_ = type_id});
  }
  return ExprResult{.terminated_ = false, .operand_ = result};
}

Operand IRGenerator::generateEquality(semantic::TypeId type_id,
                                      Operand lhs_operand, Operand rhs_operand,
                                      semantic::SemanticContext &semantic_ctx)
{
}

semantic::TypeId
IRGenerator::getDereferencedTypeId(semantic::TypeId type_id,
                                   semantic::DerefKind deref_kind,
                                   semantic::SemanticContext &semantic_ctx)
{
  const auto &type = semantic_ctx.getType(type_id);
  if (deref_kind == semantic::DerefKind::Reference)
  {
    return dynamic_cast<const semantic::ReferenceSemType *>(&type)
        ->inner_type_id_;
  }
  return dynamic_cast<const semantic::BoxSemType *>(&type)->element_type_id_;
}

Operand IRGenerator::applyCoercion(Operand operand,
                                   const semantic::ExprInfo &expr_info)
{
  for (std::size_t i = 0; i + 1 < expr_info.coercion_derefs_.size(); i++)
  {
    // 最后一步解引用不执行
    auto ptr = std::get_if<Value>(&operand);
    operand = emitLoad(module_.ir_type_table_.getOrCreatePointerType(), *ptr);
  }
  return operand;
}

ExprResult IRGenerator::generateLetStmt(const LetStmt *let_stmt,
                                        semantic::SemanticContext &semantic_ctx)
{
  ExprResult initial_result = generateExpr(let_stmt->expr_.get(), semantic_ctx);
  if (initial_result.terminated_)
  {
    return initial_result;
  }
  semantic::LocalId local_id = semantic_ctx.let_to_local_ids_.at(let_stmt);
  createLocal(local_id, initial_result.operand_.value());
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

ExprResult
IRGenerator::generateBlockExpr(const BlockExpr *block_expr,
                               semantic::SemanticContext &semantic_ctx)
{
  const Expr *tail_expr = block_expr->tail_expr_.get();
  std::size_t stmt_count = block_expr->stmts_.size();
  // 确定尾表达式
  if (!tail_expr && stmt_count > 0)
  {
    const auto *last_stmt = dynamic_cast<const ExprStmt *>(
        block_expr->stmts_[stmt_count - 1].get());
    if (last_stmt && !last_stmt->has_semicolon_)
    {
      tail_expr = last_stmt->expr_.get();
      stmt_count--;
    }
  }
  for (std::size_t i = 0; i < stmt_count; i++)
  {
    const Stmt *stmt = block_expr->stmts_[i].get();
    ExprResult result;
    if (const auto *let_stmt = dynamic_cast<const LetStmt *>(stmt))
    {
      result = generateLetStmt(let_stmt, semantic_ctx);
    }
    else
    {
      const auto *expr_stmt = dynamic_cast<const ExprStmt *>(stmt);
      result = generateExpr(expr_stmt->expr_.get(), semantic_ctx);
    }
    // 终止则返回
    if (result.terminated_)
    {
      return ExprResult{.terminated_ = true, .operand_ = std::nullopt};
    }
  }
  if (tail_expr)
  {
    return generateExpr(tail_expr, semantic_ctx);
  }
  return ExprResult{.terminated_ = false, .operand_ = std::nullopt};
}

void IRGenerator::generateFunctionBody(semantic::SemanticContext &semantic_ctx)
{
  for (auto &it : module_.functions_)
  {
    auto now_function_id = it.first;
    auto &now_function = it.second;
    const auto &func_info = semantic_ctx.functions_[now_function_id];
    if (func_info.is_builtin_) // 跳过内置函数
    {
      continue;
    }
    current_function_id_ = now_function_id;
    localid_to_place_.clear();
    current_block_id_ = std::nullopt;
    current_block_index_ = std::nullopt;
    BlockId now_block_id = newBlock();
    setInsertPoint(now_block_id);
    // 处理参数变量初始化
    const auto &local_ids =
        semantic_ctx.function_id_to_func_local_ids_.at(now_function_id);
    for (std::size_t i = 0; i < now_function.arg_values_.size(); i++)
    {
      auto arg_value = now_function.arg_values_[i];
      semantic::LocalId local_id = local_ids.param_local_ids_[i];
      createLocal(local_id, arg_value);
    }
    // 生成函数体blockExpr
    const FuncItem *func_item = func_info.declaration_;
    auto expr_result =
        generateBlockExpr(func_item->block_expr_.get(), semantic_ctx);
    if (!expr_result.terminated_) // 正常结束，补生成返回指令
    {
      emitReturn(expr_result.operand_);
    }
  }
}

void IRGenerator::createLocal(semantic::LocalId local_id,
                              Operand initial_operand)
{
  Value place = emitAlloca(getOperandTypeId(initial_operand));
  emitStore(initial_operand, place);
  localid_to_place_.emplace(local_id, place);
}

Value IRGenerator::emitAlloca(IRTypeId type_id)
{
  AllocaInst alloca_inst;
  alloca_inst.type_id_ = type_id;
  alloca_inst.return_ptr_ =
      newValue(module_.ir_type_table_.getOrCreatePointerType());
  emit(alloca_inst);
  return alloca_inst.return_ptr_;
}
Value IRGenerator::emitLoad(IRTypeId type_id, Value ptr)
{
  LoadInst load_inst;
  load_inst.type_id_ = type_id;
  load_inst.ptr_ = ptr;
  load_inst.return_value_ = newValue(type_id);
  emit(load_inst);
  return load_inst.return_value_;
}
void IRGenerator::emitStore(Operand operand, Value ptr)
{
  StoreInst store_inst;
  store_inst.operand_ = operand;
  store_inst.ptr_ = ptr;
  store_inst.type_id_ = getOperandTypeId(operand);
  emit(store_inst);
}
Value IRGenerator::emitBinary(BinaryOp op, Operand lhs_operand,
                              Operand rhs_operand)
{
  IRTypeId type_id = getOperandTypeId(lhs_operand);
  Value result = newValue(type_id);
  emit(BinaryInst{.return_value_ = result,
                  .op_ = op,
                  .type_id_ = type_id,
                  .lhs_operand_ = lhs_operand,
                  .rhs_operand_ = rhs_operand});
  return result;
}

Value IRGenerator::emitCompare(CompareOp op, Operand lhs_operand,
                               Operand rhs_operand)
{
  Value result = newValue(module_.ir_type_table_.getOrCreateIntegerType(1));
  emit(CompareInst{.return_value_ = result,
                   .op_ = op,
                   .type_id_ = getOperandTypeId(lhs_operand),
                   .lhs_operand_ = lhs_operand,
                   .rhs_operand_ = rhs_operand});
  return result;
}

Value IRGenerator::emitCast(CastOp op, Operand operand, IRTypeId to_type_id)
{
  Value result = newValue(to_type_id);
  emit(CastInst{.return_value_ = result,
                .op_ = op,
                .from_type_id_ = getOperandTypeId(operand),
                .from_operand_ = operand,
                .to_type_id_ = to_type_id});
  return result;
}

Value IRGenerator::emitGetElementPtr(IRTypeId type_id, Value ptr,
                                     std::vector<Operand> indices)
{
  Value result = newValue(module_.ir_type_table_.getOrCreatePointerType());
  emit(GetElementPtrInst{.return_ptr_ = result,
                         .type_id_ = type_id,
                         .ptr_ = ptr,
                         .indexs_ = std::move(indices)});
  return result;
}

Value IRGenerator::emitPhi(IRTypeId type_id, std::vector<PhiIncoming> incomings)
{
  Value result = newValue(type_id);
  emit(PhiInst{.return_value_ = result, .incomings_ = std::move(incomings)});
  return result;
}

void IRGenerator::emitJump(BlockId target_block_id)
{
  JumpInst jump_inst;
  jump_inst.target_block_id_ = target_block_id;
  emit(jump_inst);
}

void IRGenerator::emitBranch(Operand condition, BlockId true_block_id,
                             BlockId false_block_id)
{
  emit(BranchInst{.cond_operand_ = condition,
                  .true_block_id_ = true_block_id,
                  .false_block_id_ = false_block_id});
}

void IRGenerator::emitReturn(std::optional<Operand> operand)
{
  emit(ReturnInst{.return_operand_ = std::move(operand)});
}

} // namespace ir
