#include "AstBuilder.h"

#include <stdexcept>
#include <utility>
#include <vector>

std::unique_ptr<PathExpr>
AstBuilder::buildPathExpr(rx::RxParser::PathInExpressionContext *ctx)
{
  std::vector<PathSegment> path;
  for (auto type_path_seg : ctx->pathExprSegment())
  {
    path.push_back(buildPathExprSegment(type_path_seg));
  }
  return std::make_unique<PathExpr>(std::move(path));
}

Path AstBuilder::buildExpressionPath(rx::RxParser::PathInExpressionContext *ctx)
{
  std::vector<PathSegment> path;
  for (auto type_path_seg : ctx->pathExprSegment())
  {
    path.push_back(buildPathExprSegment(type_path_seg));
  }
  return Path(std::move(path));
}

PathSegment
AstBuilder::buildPathExprSegment(rx::RxParser::PathExprSegmentContext *ctx)
{
  std::string name = ctx->pathIdentSegment()->getText();
  if (ctx->genericArgs())
  {
    std::vector<GenericArg> generic_args;
    for (auto generic_arg : ctx->genericArgs()->genericArg())
    {
      generic_args.push_back(buildGenericArg(generic_arg));
    }
    return PathSegment(name, std::move(generic_args));
  }
  return PathSegment(name, std::vector<GenericArg>{});
}

std::unique_ptr<Type> AstBuilder::buildType(rx::RxParser::TypeRefContext *ctx)
{
  if (ctx->typeRef())
  {
    return buildType(ctx->typeRef());
  }
  else if (ctx->LPAREN())
  {
    return std::make_unique<UnitType>();
  }
  else if (ctx->typePath())
  {
    return buildPathType(ctx->typePath());
  }
  else if (ctx->referenceType())
  {
    return buildReferenceType(ctx->referenceType());
  }
  else
  {
    return buildArrayType(ctx->arrayType());
  }
}

std::unique_ptr<PathType>
AstBuilder::buildPathType(rx::RxParser::TypePathContext *ctx)
{
  std::vector<PathSegment> path;
  for (auto type_path_seg : ctx->typePathSegment())
  {
    path.push_back(buildTypePathSegment(type_path_seg));
  }
  return std::make_unique<PathType>(std::move(path));
}

std::unique_ptr<ReferenceType>
AstBuilder::buildReferenceType(rx::RxParser::ReferenceTypeContext *ctx)
{
  std::optional<std::string> lifetime_arg = std::nullopt;
  bool mut_arg = false;
  if (ctx->lifetime())
  {
    lifetime_arg = ctx->lifetime()->getText();
  }
  if (ctx->MUT())
  {
    mut_arg = true;
  }
  if (ctx->AMP())
  {
    return std::make_unique<ReferenceType>(buildType(ctx->typeRef()), mut_arg,
                                           lifetime_arg);
  }
  else
  {
    return std::make_unique<ReferenceType>(
        std::make_unique<ReferenceType>(buildType(ctx->typeRef()), mut_arg,
                                        lifetime_arg),
        false, std::nullopt);
  }
}

std::unique_ptr<ArrayType>
AstBuilder::buildArrayType(rx::RxParser::ArrayTypeContext *ctx)
{
  std::unique_ptr<Type> type_ptr = buildType(ctx->typeRef());
  std::unique_ptr<Expr> const_value = buildConstValue(ctx->constValue());
  return std::make_unique<ArrayType>(std::move(type_ptr),
                                     std::move(const_value));
}

PathSegment
AstBuilder::buildTypePathSegment(rx::RxParser::TypePathSegmentContext *ctx)
{
  std::string name = ctx->pathIdentSegment()->getText();
  if (ctx->genericArgs())
  {
    std::vector<GenericArg> generic_args;
    for (auto generic_arg : ctx->genericArgs()->genericArg())
    {
      generic_args.push_back(buildGenericArg(generic_arg));
    }
    return PathSegment(name, std::move(generic_args));
  }
  return PathSegment(name, std::vector<GenericArg>{});
}

GenericArg AstBuilder::buildGenericArg(rx::RxParser::GenericArgContext *ctx)
{
  if (ctx->lifetime())
  {
    return GenericArg(false, nullptr, ctx->lifetime()->getText());
  }
  else
  {
    return GenericArg(true, buildType(ctx->typeRef()), std::nullopt);
  }
}

std::unique_ptr<Expr>
AstBuilder::buildConstValue(rx::RxParser::ConstValueContext *ctx)
{
  if (ctx->INTEGER_LITERAL())
  {
    return std::make_unique<LiteralExpr>(
        true, false,
        LiteralStringToInt(ctx->INTEGER_LITERAL()->getText()),
        LiteralStringToIntegerType(ctx->INTEGER_LITERAL()->getText()));
  }
  else if (ctx->TRUE())
  {
    return std::make_unique<LiteralExpr>(false, true, 0, IntegerType::Inferred);
  }
  else if (ctx->FALSE())
  {
    return std::make_unique<LiteralExpr>(false, false, 0,
                                         IntegerType::Inferred);
  }
  else if (ctx->pathInExpression())
  {
    return buildPathExpr(ctx->pathInExpression());
  }
  else if (ctx->MINUS())
  {
    return std::make_unique<UnaryExpr>(UnaryOperator::Negate,
                                       buildMagnitude(ctx->magnitude()));
  }
  else
  {
    return buildConstValue(ctx->constValue());
  }
}

std::unique_ptr<Expr>
AstBuilder::buildMagnitude(rx::RxParser::MagnitudeContext *ctx)
{
  if (ctx->INTEGER_LITERAL())
  {
    return std::make_unique<LiteralExpr>(
        true, false,
        LiteralStringToInt(ctx->INTEGER_LITERAL()->getText()),
        LiteralStringToIntegerType(ctx->INTEGER_LITERAL()->getText()));
  }
  else if (ctx->pathInExpression())
  {
    return buildPathExpr(ctx->pathInExpression());
  }
  else
  {
    return buildMagnitude(ctx->magnitude());
  }
}

std::unique_ptr<Crate> AstBuilder::Build(rx::RxParser::CrateContext *ctx)
{
  result_ = std::move(buildCrate(ctx));
  return std::move(result_);
}

std::unique_ptr<Crate> AstBuilder::buildCrate(rx::RxParser::CrateContext *ctx)
{
  std::vector<std::unique_ptr<Item>> items;
  items.reserve(ctx->item().size());
  for (auto *ctx_item : ctx->item())
  {
    items.push_back(std::move(buildItem(ctx_item)));
  }
  return std::make_unique<Crate>(std::move(items));
}

std::unique_ptr<Item> AstBuilder::buildItem(rx::RxParser::ItemContext *ctx)
{
  if (ctx->functionDefinition()) // func def
  {
    return buildFuncItem(ctx->functionDefinition());
  }
  // to be continue
}

std::unique_ptr<FuncItem>
AstBuilder::buildFuncItem(rx::RxParser::FunctionDefinitionContext *ctx)
{
  std::string ident = ctx->identifier()->getText();
  std::unique_ptr<BlockExpr> block_expr =
      std::move(buildBlockExpr(ctx->blockExpression()));
  return std::make_unique<FuncItem>(std::move(ident), std::move(block_expr));
}

std::unique_ptr<BlockExpr>
AstBuilder::buildBlockExpr(rx::RxParser::BlockExpressionContext *ctx)
{
  std::vector<rx::RxParser::StatementContext *> stmt_ctxs = ctx->statement();
  auto expr_stmt_ctx = ctx->statementExpression();
  std::vector<std::unique_ptr<Stmt>> stmts;
  for (auto &stmt_ctx : stmt_ctxs)
  {
    stmts.push_back(std::move(buildStmt(stmt_ctx)));
  }
  if (expr_stmt_ctx)
  {
    stmts.push_back(std::move(buildExprStmt(expr_stmt_ctx)));
  }
  return std::make_unique<BlockExpr>(std::move(stmts));
}

std::unique_ptr<Stmt> AstBuilder::buildStmt(rx::RxParser::StatementContext *ctx)
{
  if (ctx->letStatement()) // let statement
  {
    return buildLetStmt(ctx->letStatement());
  }
  // to be continue
}

std::unique_ptr<LetStmt>
AstBuilder::buildLetStmt(rx::RxParser::LetStatementContext *ctx)
{
  std::string ident = ctx->identifierBinding()->identifier()->getText();
  bool mut = (ctx->identifierBinding()->MUT() != nullptr);
  std::unique_ptr<Type> type;
  if (ctx->typeRef())
  {
    type = buildType(ctx->typeRef());
  }
  std::unique_ptr<Expr> expr = buildExpr(ctx->expression());
  return std::make_unique<LetStmt>(std::move(ident), mut, std::move(type),
                                   std::move(expr));
}

std::unique_ptr<Expr>
AstBuilder::buildExpr(rx::RxParser::ExpressionContext *ctx)
{
  return buildAssignExpr(ctx->assignmentExpression());
}

std::unique_ptr<ExprStmt>
AstBuilder::buildExprStmt(rx::RxParser::StatementExpressionContext *ctx)
{
}
AssignmentOperator
AstBuilder::getAssignOperator(rx::RxParser::AssignmentOperatorContext *ctx)
{
  if (ctx->equalsSign())
  {
    return AssignmentOperator::Assign;
  }
  if (ctx->PLUS_ASSIGN())
  {
    return AssignmentOperator::AddAssign;
  }
  if (ctx->MINUS_ASSIGN())
  {
    return AssignmentOperator::SubtractAssign;
  }
  if (ctx->STAR_ASSIGN())
  {
    return AssignmentOperator::MultiplyAssign;
  }
  if (ctx->SLASH_ASSIGN())
  {
    return AssignmentOperator::DivideAssign;
  }
  if (ctx->PERCENT_ASSIGN())
  {
    return AssignmentOperator::RemainderAssign;
  }
  if (ctx->AMP_ASSIGN())
  {
    return AssignmentOperator::BitAndAssign;
  }
  if (ctx->PIPE_ASSIGN())
  {
    return AssignmentOperator::BitOrAssign;
  }
  if (ctx->CARET_ASSIGN())
  {
    return AssignmentOperator::BitXorAssign;
  }
  if (ctx->SHL_ASSIGN())
  {
    return AssignmentOperator::ShiftLeftAssign;
  }
  // >>= is represented by GT GT_SECOND SHR_EQ.
  if (ctx->GT() && ctx->GT_SECOND() && ctx->SHR_EQ())
  {
    return AssignmentOperator::ShiftRightAssign;
  }
}

std::unique_ptr<Expr>
AstBuilder::buildAssignExpr(rx::RxParser::AssignmentExpressionContext *ctx)
{
  if (ctx->assignmentOperator())
  {
    return std::make_unique<AssignExpr>(
        getAssignOperator(ctx->assignmentOperator()),
        buildOrExpr(ctx->logicalOrExpression()), buildExpr(ctx->expression()));
  }
  return buildOrExpr(ctx->logicalOrExpression());
}
std::unique_ptr<Expr>
AstBuilder::buildOrExpr(rx::RxParser::LogicalOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildAndExpr(ctx->logicalAndExpression()[0]);
  for (int i = 1; i < ctx->logicalAndExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalOr, std::move(result_ptr),
        buildAndExpr(ctx->logicalAndExpression()[i]));
  }
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildAndExpr(rx::RxParser::LogicalAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildCompExpr(ctx->comparisonExpression()[0]);
  for (int i = 1; i < ctx->comparisonExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalAnd, std::move(result_ptr),
        buildCompExpr(ctx->comparisonExpression()[i]));
  }
  return result_ptr;
}
BinaryOperator
AstBuilder::getCompOperator(rx::RxParser::ComparisonExceptLtContext *ctx)
{
  if (ctx->EQEQ())
  {
    return BinaryOperator::Equal;
  }
  if (ctx->NE())
  {
    return BinaryOperator::NotEqual;
  }
  if (ctx->LE())
  {
    return BinaryOperator::LessEqual;
  }
  if (ctx->GE_EQ() || ctx->SHR_EQ())
  {
    return BinaryOperator::GreaterEqual;
  }
  return BinaryOperator::Greater;
}

std::unique_ptr<Expr>
AstBuilder::buildCompExpr(rx::RxParser::ComparisonExpressionContext *ctx)
{
  if (ctx->LT())
  {
    return std::make_unique<BinaryExpr>(
        BinaryOperator::Less, buildBitOrExpr(ctx->closedBitOrExpression()),
        buildBitOrExpr(ctx->bitOrExpression()[0]));
  }
  if (ctx->comparisonExceptLt())
  {
    return std::make_unique<BinaryExpr>(
        getCompOperator(ctx->comparisonExceptLt()),
        buildBitOrExpr(ctx->bitOrExpression()[0]),
        buildBitOrExpr(ctx->bitOrExpression()[1]));
  }
  return buildBitOrExpr(ctx->bitOrExpression()[0]);
}
std::unique_ptr<Expr>
AstBuilder::buildBitOrExpr(rx::RxParser::BitOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildBitXorExpr(ctx->bitXorExpression()[0]);
  for (int i = 1; i < ctx->bitXorExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildBitXorExpr(ctx->bitXorExpression()[i]));
  }
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitOrExpr(rx::RxParser::ClosedBitOrExpressionContext *ctx)
{
  if (ctx->PIPE().size() == 0)
  {
    return buildBitXorExpr(ctx->closedBitXorExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildBitXorExpr(ctx->bitXorExpression()[0]);
  for (int i = 1; i < ctx->bitXorExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildBitXorExpr(ctx->bitXorExpression()[i]));
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitOr, std::move(result_ptr),
      buildBitXorExpr(ctx->closedBitXorExpression()));
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitXorExpr(rx::RxParser::BitXorExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildBitAndExpr(ctx->bitAndExpression()[0]);
  for (int i = 1; i < ctx->bitAndExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildBitAndExpr(ctx->bitAndExpression()[i]));
  }
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitXorExpr(rx::RxParser::ClosedBitXorExpressionContext *ctx)
{
  if (ctx->CARET().size() == 0)
  {
    return buildBitAndExpr(ctx->closedBitAndExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildBitAndExpr(ctx->bitAndExpression()[0]);
  for (int i = 1; i < ctx->bitAndExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildBitAndExpr(ctx->bitAndExpression()[i]));
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitXor, std::move(result_ptr),
      buildBitAndExpr(ctx->closedBitAndExpression()));
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitAndExpr(rx::RxParser::BitAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr = buildShiftExpr(ctx->shiftExpression()[0]);
  for (int i = 1; i < ctx->shiftExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildShiftExpr(ctx->shiftExpression()[i]));
  }
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitAndExpr(rx::RxParser::ClosedBitAndExpressionContext *ctx)
{
  if (ctx->AMP().size() == 0)
  {
    return buildShiftExpr(ctx->closedShiftExpression());
  }
  std::unique_ptr<Expr> result_ptr = buildShiftExpr(ctx->shiftExpression()[0]);
  for (int i = 1; i < ctx->shiftExpression().size(); i++)
  {
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildShiftExpr(ctx->shiftExpression()[i]));
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitAnd, std::move(result_ptr),
      buildShiftExpr(ctx->closedShiftExpression()));
  return result_ptr;
}

std::unique_ptr<Expr>
AstBuilder::buildShiftExpr(rx::RxParser::ShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::AdditiveExpressionContext *>(child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::AdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::ClosedAdditiveExpressionContext *>(
                 child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ClosedAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}
std::unique_ptr<Expr>
AstBuilder::buildShiftExpr(rx::RxParser::ClosedShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::AdditiveExpressionContext *>(child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::AdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::ClosedAdditiveExpressionContext *>(
                 child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ClosedAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}
std::unique_ptr<Expr>
AstBuilder::buildAddExpr(rx::RxParser::AdditiveExpressionContext *ctx)
{
  auto mult_expr_contexts = ctx->multiplicativeExpression();
  auto operators = ctx->additiveOperator();

  std::unique_ptr<Expr> result_expr = buildMultExpr(mult_expr_contexts[0]);
  for (int i = 1; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i - 1]);
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildMultExpr(mult_expr_contexts[i]));
  }
  return result_expr;
}
std::unique_ptr<Expr>
AstBuilder::buildAddExpr(rx::RxParser::ClosedAdditiveExpressionContext *ctx)
{
  auto mult_expr_contexts = ctx->multiplicativeExpression();
  auto operators = ctx->additiveOperator();
  if (!operators.size())
  {
    return buildMultExpr(ctx->closedMultiplicativeExpression());
  }
  std::unique_ptr<Expr> result_expr = buildMultExpr(mult_expr_contexts[0]);
  for (int i = 1; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i - 1]);
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildMultExpr(mult_expr_contexts[i]));
  }

  BinaryOperator last_op = getAddOperator(operators.back());
  return std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildMultExpr(ctx->closedMultiplicativeExpression()));
}

BinaryOperator
AstBuilder::getAddOperator(rx::RxParser::AdditiveOperatorContext *ctx)
{
  if (ctx->PLUS())
  {
    return BinaryOperator::Add;
  }
  return BinaryOperator::Subtract;
}

BinaryOperator
AstBuilder::getMultOperator(rx::RxParser::MultiplicativeOperatorContext *ctx)
{
  if (ctx->STAR())
  {
    return BinaryOperator::Multiply;
  }
  if (ctx->SLASH())
  {
    return BinaryOperator::Divide;
  }
  return BinaryOperator::Remainder;
}

std::unique_ptr<Expr>
AstBuilder::buildMultExpr(rx::RxParser::MultiplicativeExpressionContext *ctx)
{
  if (ctx->multiplicativeOperator().size())
  {
    std::unique_ptr<Expr> result_expr = buildCastExpr(ctx->castExpression()[0]);
    for (int i = 1; i < ctx->castExpression().size(); i++)
    {
      result_expr = std::make_unique<BinaryExpr>(
          getMultOperator(ctx->multiplicativeOperator()[i - 1]),
          std::move(result_expr), buildCastExpr(ctx->castExpression()[i]));
    }
    return std::move(result_expr);
  }
  return buildCastExpr(ctx->castExpression()[0]);
}
std::unique_ptr<Expr> AstBuilder::buildMultExpr(
    rx::RxParser::ClosedMultiplicativeExpressionContext *ctx)
{
  auto cast_expr_contexts = ctx->castExpression();
  auto operators = ctx->multiplicativeOperator();
  if (!operators.size())
  {
    return buildCastExpr(ctx->closedCastExpression());
  }
  std::unique_ptr<Expr> result_expr = buildCastExpr(cast_expr_contexts[0]);
  for (int i = 1; i < cast_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getMultOperator(operators[i - 1]);
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildCastExpr(cast_expr_contexts[i]));
  }
  BinaryOperator last_op = getMultOperator(operators[operators.size() - 1]);
  return std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildCastExpr(ctx->closedCastExpression()));
}
std::unique_ptr<Expr>
AstBuilder::buildCastExpr(rx::RxParser::CastExpressionContext *ctx)
{
  if (ctx->AS().size())
  {
    // to be continue
  }
  return buildUnaryExpr(ctx->unaryExpression());
}
std::unique_ptr<Expr>
AstBuilder::buildCastExpr(rx::RxParser::ClosedCastExpressionContext *ctx)
{
  if (ctx->AS())
  {
    // to be continue
  }
  return buildUnaryExpr(ctx->unaryExpression());
}
std::unique_ptr<Expr>
AstBuilder::buildUnaryExpr(rx::RxParser::UnaryExpressionContext *ctx)
{
  if (ctx->unaryOperator())
  {
    auto op = ctx->unaryOperator();
    auto expr = buildUnaryExpr(ctx->unaryExpression());
    if (op->MINUS())
    {
      return std::make_unique<UnaryExpr>(UnaryOperator::Negate,
                                         std::move(expr));
    }
    else if (op->NOT())
    {
      return std::make_unique<UnaryExpr>(UnaryOperator::Not, std::move(expr));
    }
    else if (op->STAR())
    {
      return std::make_unique<UnaryExpr>(UnaryOperator::Dereference,
                                         std::move(expr));
    }
    else if (op->AMP())
    {
      UnaryOperator kind =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      return std::make_unique<UnaryExpr>(kind, std::move(expr));
    }
    else
    {
      UnaryOperator kind_in =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      return std::make_unique<UnaryExpr>(
          UnaryOperator::Borrow,
          std::make_unique<UnaryExpr>(kind_in, std::move(expr)));
    }
  }
  else
  {
    return buildPostfixExpr(ctx->postfixExpression());
  }
}

std::unique_ptr<Expr>
AstBuilder::buildPostfixExpr(rx::RxParser::PostfixExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_expr =
      buildPrimaryExpr(ctx->primaryExpression());
  auto postfixs = ctx->postfixSuffix();
  for (int i = 0; i < postfixs.size(); i++)
  {
    if (postfixs[i]->callArguments())
    {
      std::vector<std::unique_ptr<Expr>> arguments;
      for (auto expr : postfixs[i]->callArguments()->expression())
      {
        arguments.push_back(buildExpr(expr));
      }
      result_expr = std::make_unique<CallExpr>(std::move(result_expr),
                                               std::move(arguments));
    }
    else if (postfixs[i]->expression())
    {
      std::unique_ptr<Expr> index_expr = buildExpr(postfixs[i]->expression());
      result_expr = std::make_unique<IndexExpr>(std::move(result_expr),
                                                std::move(index_expr));
    }
    else
    {
      // to be continue
    }
  }
  return result_expr;
}

std::unique_ptr<Expr>
AstBuilder::buildPrimaryExpr(rx::RxParser::PrimaryExpressionContext *ctx)
{
  if (ctx->nonBlockPrimary())
  {
    return buildNonBlockPrimary(ctx->nonBlockPrimary());
  }
  // to be continue
}

std::unique_ptr<Expr>
AstBuilder::buildNonBlockPrimary(rx::RxParser::NonBlockPrimaryContext *ctx)
{
  if (ctx->literalExpression())
  {
    return buildLiteralExpr(ctx->literalExpression());
  }
  // to be continue
}

std::unique_ptr<LiteralExpr>
AstBuilder::buildLiteralExpr(rx::RxParser::LiteralExpressionContext *ctx)
{
  if (ctx->INTEGER_LITERAL())
  {
    return std::make_unique<LiteralExpr>(
        true, true, LiteralStringToInt(ctx->INTEGER_LITERAL()->getText()),
        LiteralStringToIntegerType(ctx->INTEGER_LITERAL()->getText()));
  }
  else if (ctx->FALSE())
  {
    return std::make_unique<LiteralExpr>(false, false, 0,
                                         IntegerType::Inferred);
  }
  else
  {
    return std::make_unique<LiteralExpr>(false, true, 0, IntegerType::Inferred);
  }
}
