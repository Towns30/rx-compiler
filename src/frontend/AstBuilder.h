#pragma once

#include <any>
#include <memory>

#include "../../generated/RxParser.h"
#include "../../generated/RxParserBaseVisitor.h"
#include "../Utils.h"
#include "../ast/Ast.h"

class AstBuilder : public rx::RxParserBaseVisitor
{
public:
  std::unique_ptr<Crate> Build(rx::RxParser::CrateContext *ctx);

private:
  std::unique_ptr<Crate> buildCrate(rx::RxParser::CrateContext *ctx);
  std::unique_ptr<Item> buildItem(rx::RxParser::ItemContext *ctx);
  std::unique_ptr<Stmt> buildStmt(rx::RxParser::StatementContext *ctx);
  std::unique_ptr<Expr> buildExpr(rx::RxParser::ExpressionContext *ctx);
  std::unique_ptr<FuncItem>
  buildFuncItem(rx::RxParser::FunctionDefinitionContext *ctx);
  std::unique_ptr<BlockExpr>
  buildBlockExpr(rx::RxParser::BlockExpressionContext *ctx);
  std::unique_ptr<LetStmt> buildLetStmt(rx::RxParser::LetStatementContext *ctx);
  std::unique_ptr<ExprStmt>
  buildExprStmt(rx::RxParser::StatementExpressionContext *ctx);
  AssignmentOperator
  getAssignOperator(rx::RxParser::AssignmentOperatorContext *ctx);
  std::unique_ptr<Expr>
  buildAssignExpr(rx::RxParser::AssignmentExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildOrExpr(rx::RxParser::LogicalOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildAndExpr(rx::RxParser::LogicalAndExpressionContext *ctx);
  BinaryOperator getCompOperator(rx::RxParser::ComparisonExceptLtContext *ctx);
  std::unique_ptr<Expr>
  buildCompExpr(rx::RxParser::ComparisonExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildBitOrExpr(rx::RxParser::BitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildBitOrExpr(rx::RxParser::ClosedBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildBitXorExpr(rx::RxParser::BitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildBitXorExpr(rx::RxParser::ClosedBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildBitAndExpr(rx::RxParser::BitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildBitAndExpr(rx::RxParser::ClosedBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildShiftExpr(rx::RxParser::ShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildShiftExpr(rx::RxParser::ClosedShiftExpressionContext *ctx);
  BinaryOperator getAddOperator(rx::RxParser::AdditiveOperatorContext *ctx);
  std::unique_ptr<Expr>
  buildAddExpr(rx::RxParser::AdditiveExpressionContext *tcx);
  std::unique_ptr<Expr>
  buildAddExpr(rx::RxParser::ClosedAdditiveExpressionContext *ctx);
  BinaryOperator
  getMultOperator(rx::RxParser::MultiplicativeOperatorContext *ctx);
  std::unique_ptr<Expr>
  buildMultExpr(rx::RxParser::MultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildMultExpr(rx::RxParser::ClosedMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr> buildCastExpr(rx::RxParser::CastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildCastExpr(rx::RxParser::ClosedCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildUnaryExpr(rx::RxParser::UnaryExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildPostfixExpr(rx::RxParser::PostfixExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildDotSuffix(std::unique_ptr<Expr> base,
                 rx::RxParser::DotSuffixContext *ctx);
  std::unique_ptr<Expr>
  buildPrimaryExpr(rx::RxParser::PrimaryExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildNonBlockPrimary(rx::RxParser::NonBlockPrimaryContext *ctx);
  std::vector<StructField>
  buildStructFields(rx::RxParser::StructExprFieldsContext *ctx);
  StructField
  buildStructField(rx::RxParser::StructExprFieldContext *ctx);
  std::unique_ptr<LiteralExpr>
  buildLiteralExpr(rx::RxParser::LiteralExpressionContext *ctx);
  std::unique_ptr<Type> buildType(rx::RxParser::TypeRefContext *ctx);
  std::unique_ptr<PathType> buildPathType(rx::RxParser::TypePathContext *ctx);
  std::unique_ptr<ReferenceType>
  buildReferenceType(rx::RxParser::ReferenceTypeContext *ctx);
  std::unique_ptr<ArrayType>
  buildArrayType(rx::RxParser::ArrayTypeContext *ctx);
  PathSegment buildTypePathSegment(rx::RxParser::TypePathSegmentContext *ctx);
  GenericArg buildGenericArg(rx::RxParser::GenericArgContext *ctx);
  std::unique_ptr<Expr> buildConstValue(rx::RxParser::ConstValueContext *ctx);
  std::unique_ptr<Expr> buildMagnitude(rx::RxParser::MagnitudeContext *ctx);
  std::unique_ptr<PathExpr>
  buildPathExpr(rx::RxParser::PathInExpressionContext *ctx);
  Path buildExpressionPath(rx::RxParser::PathInExpressionContext *ctx);
  PathSegment buildPathExprSegment(rx::RxParser::PathExprSegmentContext *ctx);
  std::unique_ptr<Type> buildClosedCastType(rx::RxParser::ClosedCastTypeContext *ctx);
  std::unique_ptr<Expr> buildExprWithBlock(rx::RxParser::ExpressionWithBlockContext *ctx);
  
  
  
  std::unique_ptr<Expr>
  buildConditionExpr(rx::RxParser::ConditionExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionAssignExpr(rx::RxParser::ConditionAssignmentExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionOrExpr(rx::RxParser::ConditionLogicalOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionAndExpr(rx::RxParser::ConditionLogicalAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionCompExpr(rx::RxParser::ConditionComparisonExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBitOrExpr(rx::RxParser::ConditionBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBitOrExpr(rx::RxParser::ConditionClosedBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBitXorExpr(rx::RxParser::ConditionBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBitXorExpr(rx::RxParser::ConditionClosedBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBitAndExpr(rx::RxParser::ConditionBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBitAndExpr(rx::RxParser::ConditionClosedBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionShiftExpr(rx::RxParser::ConditionShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionShiftExpr(rx::RxParser::ConditionClosedShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionAddExpr(rx::RxParser::ConditionAdditiveExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionAddExpr(rx::RxParser::ConditionClosedAdditiveExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionMultExpr(rx::RxParser::ConditionMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionMultExpr(rx::RxParser::ConditionClosedMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionCastExpr(rx::RxParser::ConditionCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionCastExpr(rx::RxParser::ConditionClosedCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionUnaryExpr(rx::RxParser::ConditionUnaryExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionPostfixExpr(rx::RxParser::ConditionPostfixExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionPrimaryExpr(rx::RxParser::ConditionPrimaryContext *ctx);
  std::unique_ptr<Expr>
  buildConditionPrimaryExprWithoutBareBlock(rx::RxParser::ConditionPrimaryWithoutBareBlockContext *ctx);
  std::unique_ptr<ArrayExpr>
  buildArrayExpr(rx::RxParser::ArrayExpressionContext *ctx);
  std::unique_ptr<IfExpr>
  buildIfExpr(rx::RxParser::IfExpressionContext *ctx);

  std::unique_ptr<Expr>
  buildConditionBreakExpr(rx::RxParser::ConditionBreakExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakAssignExpr(rx::RxParser::ConditionBreakAssignmentExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakOrExpr(rx::RxParser::ConditionBreakLogicalOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakAndExpr(rx::RxParser::ConditionBreakLogicalAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakCompExpr(rx::RxParser::ConditionBreakComparisonExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakBitOrExpr(rx::RxParser::ConditionBreakBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakBitOrExpr(rx::RxParser::ConditionBreakClosedBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakBitXorExpr(rx::RxParser::ConditionBreakBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakBitXorExpr(rx::RxParser::ConditionBreakClosedBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakBitAndExpr(rx::RxParser::ConditionBreakBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakBitAndExpr(rx::RxParser::ConditionBreakClosedBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakShiftExpr(rx::RxParser::ConditionBreakShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakShiftExpr(rx::RxParser::ConditionBreakClosedShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakAddExpr(rx::RxParser::ConditionBreakAdditiveExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakAddExpr(rx::RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakMultExpr(rx::RxParser::ConditionBreakMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakMultExpr(rx::RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakCastExpr(rx::RxParser::ConditionBreakCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakCastExpr(rx::RxParser::ConditionBreakClosedCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakUnaryExpr(rx::RxParser::ConditionBreakUnaryExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildConditionBreakPostfixExpr(rx::RxParser::ConditionBreakPostfixExpressionContext *ctx);
  
  std::unique_ptr<Expr>
  buildStmtExpr(rx::RxParser::StatementExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtAssignExpr(rx::RxParser::StatementAssignmentExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtOrExpr(rx::RxParser::StatementLogicalOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtAndExpr(rx::RxParser::StatementLogicalAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtCompExpr(rx::RxParser::StatementComparisonExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtBitOrExpr(rx::RxParser::StatementBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtBitOrExpr(rx::RxParser::StatementClosedBitOrExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtBitXorExpr(rx::RxParser::StatementBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtBitXorExpr(rx::RxParser::StatementClosedBitXorExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtBitAndExpr(rx::RxParser::StatementBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtBitAndExpr(rx::RxParser::StatementClosedBitAndExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtShiftExpr(rx::RxParser::StatementShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtShiftExpr(rx::RxParser::StatementClosedShiftExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtAddExpr(rx::RxParser::StatementAdditiveExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtAddExpr(rx::RxParser::StatementClosedAdditiveExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtMultExpr(rx::RxParser::StatementMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtMultExpr(rx::RxParser::StatementClosedMultiplicativeExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtCastExpr(rx::RxParser::StatementCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtCastExpr(rx::RxParser::StatementClosedCastExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtUnaryExpr(rx::RxParser::StatementUnaryExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildStmtPostfixExpr(rx::RxParser::StatementPostfixExpressionContext *ctx);

  std::unique_ptr<Crate> result_;
};
