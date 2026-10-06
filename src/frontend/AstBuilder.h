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
  buildPrimaryExpr(rx::RxParser::PrimaryExpressionContext *ctx);
  std::unique_ptr<Expr>
  buildNonBlockPrimary(rx::RxParser::NonBlockPrimaryContext *ctx);
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
  std::unique_ptr<Crate> result_;
};
