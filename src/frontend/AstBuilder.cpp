#include "AstBuilder.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

std::optional<SourceSpan> AstBuilder::spanOf(const antlr4::Token *token)
{
  if (!token)
  {
    return std::nullopt;
  }
  const auto begin = token->getStartIndex();
  const auto invalid = std::numeric_limits<std::size_t>::max();
  if (begin == invalid)
  {
    return std::nullopt;
  }
  if (token->getType() == antlr4::Token::EOF)
  {
    return SourceSpan{begin, begin};
  }
  const auto stop = token->getStopIndex();
  if (stop == invalid || stop < begin)
  {
    return std::nullopt;
  }
  return SourceSpan{begin, stop + 1};
}

std::optional<SourceSpan>
AstBuilder::spanOf(const antlr4::ParserRuleContext *ctx)
{
  if (!ctx)
  {
    return std::nullopt;
  }
  const auto first = spanOf(ctx->getStart());
  const auto last = spanOf(ctx->getStop());
  if (!first || !last)
  {
    return std::nullopt;
  }
  if (last->end <= first->begin)
  {
    return SourceSpan{first->begin, first->begin};
  }
  return SourceSpan{first->begin, last->end};
}

SourceSpan AstBuilder::cover(SourceSpan first, SourceSpan last)
{
  return SourceSpan{std::min(first.begin, last.begin),
                    std::max(first.end, last.end)};
}

std::unique_ptr<PathExpr>
AstBuilder::buildPathExpr(rx::RxParser::PathInExpressionContext *ctx)
{
  std::vector<PathSegment> path;
  for (auto type_path_seg : ctx->pathExprSegment())
  {
    path.push_back(buildPathExprSegment(type_path_seg));
  }
  auto result = std::make_unique<PathExpr>(std::move(path));
  result->span_ = spanOf(ctx);
  return result;
}

Path AstBuilder::buildExpressionPath(rx::RxParser::PathInExpressionContext *ctx)
{
  std::vector<PathSegment> path;
  for (auto type_path_seg : ctx->pathExprSegment())
  {
    path.push_back(buildPathExprSegment(type_path_seg));
  }
  auto result = Path(std::move(path));
  result.span_ = spanOf(ctx);
  return result;
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
      if (generic_arg->typeRef())
      {
        generic_args.push_back(buildGenericArg(generic_arg));
      }
    }
    auto result = PathSegment(name, std::move(generic_args));
    result.span_ = spanOf(ctx);
    result.name_span_ = spanOf(ctx->pathIdentSegment());
    return result;
  }
  auto result = PathSegment(name, std::vector<GenericArg>{});
  result.span_ = spanOf(ctx);
  result.name_span_ = spanOf(ctx->pathIdentSegment());
  return result;
}

std::unique_ptr<Type> AstBuilder::buildType(rx::RxParser::TypeRefContext *ctx)
{
  if (ctx->typeRef())
  {
    return buildType(ctx->typeRef());
  }
  else if (ctx->LPAREN())
  {
    auto result = std::make_unique<UnitType>();
    result->span_ = spanOf(ctx);
    return result;
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
  auto result = std::make_unique<PathType>(std::move(path));
  result->span_ = spanOf(ctx);
  return result;
}

std::unique_ptr<ReferenceType>
AstBuilder::buildReferenceType(rx::RxParser::ReferenceTypeContext *ctx)
{
  bool mut_arg = false;
  if (ctx->MUT())
  {
    mut_arg = true;
  }
  if (ctx->AMP())
  {
    auto result =
        std::make_unique<ReferenceType>(buildType(ctx->typeRef()), mut_arg);
    result->span_ = spanOf(ctx);
    return result;
  }
  else
  {
    auto inner_span = spanOf(ctx);
    if (inner_span && inner_span->begin < inner_span->end)
    {
      ++inner_span->begin;
    }
    else
    {
      inner_span = std::nullopt;
    }
    auto inner_node =
        std::make_unique<ReferenceType>(buildType(ctx->typeRef()), mut_arg);
    inner_node->span_ = inner_span;
    auto result = std::make_unique<ReferenceType>(std::move(inner_node), false);
    result->span_ = spanOf(ctx);
    return result;
  }
}

std::unique_ptr<ArrayType>
AstBuilder::buildArrayType(rx::RxParser::ArrayTypeContext *ctx)
{
  std::unique_ptr<Type> type_ptr = buildType(ctx->typeRef());
  std::unique_ptr<Expr> const_value = buildConstValue(ctx->constValue());
  auto result =
      std::make_unique<ArrayType>(std::move(type_ptr), std::move(const_value));
  result->span_ = spanOf(ctx);
  return result;
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
      if (generic_arg->typeRef())
      {
        generic_args.push_back(buildGenericArg(generic_arg));
      }
    }
    auto result = PathSegment(name, std::move(generic_args));
    result.span_ = spanOf(ctx);
    result.name_span_ = spanOf(ctx->pathIdentSegment());
    return result;
  }
  auto result = PathSegment(name, std::vector<GenericArg>{});
  result.span_ = spanOf(ctx);
  result.name_span_ = spanOf(ctx->pathIdentSegment());
  return result;
}

GenericArg AstBuilder::buildGenericArg(rx::RxParser::GenericArgContext *ctx)
{
  auto result = GenericArg(buildType(ctx->typeRef()));
  result.span_ = spanOf(ctx);
  return result;
}

std::unique_ptr<Expr>
AstBuilder::buildConstValue(rx::RxParser::ConstValueContext *ctx)
{
  if (ctx->INTEGER_LITERAL())
  {
    auto result = std::make_unique<LiteralExpr>(
        true, false, LiteralStringToInt(ctx->INTEGER_LITERAL()->getText()),
        LiteralStringToIntegerType(ctx->INTEGER_LITERAL()->getText()));
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->TRUE())
  {
    auto result =
        std::make_unique<LiteralExpr>(false, true, 0, IntegerType::Inferred);
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->FALSE())
  {
    auto result =
        std::make_unique<LiteralExpr>(false, false, 0, IntegerType::Inferred);
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->pathInExpression())
  {
    return buildPathExpr(ctx->pathInExpression());
  }
  else if (ctx->MINUS())
  {
    auto result = std::make_unique<UnaryExpr>(UnaryOperator::Negate,
                                              buildMagnitude(ctx->magnitude()));
    result->span_ = spanOf(ctx);
    return result;
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
    auto result = std::make_unique<LiteralExpr>(
        true, false, LiteralStringToInt(ctx->INTEGER_LITERAL()->getText()),
        LiteralStringToIntegerType(ctx->INTEGER_LITERAL()->getText()));
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
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

std::unique_ptr<Type>
AstBuilder::buildClosedCastType(rx::RxParser::ClosedCastTypeContext *ctx)
{
  if (ctx->LPAREN())
  {
    if (!ctx->typeRef())
    {
      auto result = std::make_unique<UnitType>();
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      return buildType(ctx->typeRef());
    }
  }
  else if (ctx->arrayType())
  {
    return buildArrayType(ctx->arrayType());
  }
  else if (ctx->closedCastType())
  {
    bool mut_arg = false;
    if (ctx->MUT())
    {
      mut_arg = true;
    }
    if (ctx->AMP())
    {
      auto result = std::make_unique<ReferenceType>(
          buildClosedCastType(ctx->closedCastType()), mut_arg);
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      auto inner_span = spanOf(ctx);
      if (inner_span && inner_span->begin < inner_span->end)
      {
        ++inner_span->begin;
      }
      else
      {
        inner_span = std::nullopt;
      }
      auto inner_node = std::make_unique<ReferenceType>(
          buildClosedCastType(ctx->closedCastType()), mut_arg);
      inner_node->span_ = inner_span;
      auto result =
          std::make_unique<ReferenceType>(std::move(inner_node), false);
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    auto name = ctx->pathIdentSegment()->getText();
    std::vector<GenericArg> generic_args;
    for (auto generic_arg : ctx->genericArgs()->genericArg())
    {
      if (generic_arg->typeRef())
      {
        generic_args.push_back(buildGenericArg(generic_arg));
      }
    }
    PathSegment last_segment = PathSegment(name, std::move(generic_args));
    const auto node_span_first = spanOf(ctx->pathIdentSegment()->getStart());
    const auto node_span_last = spanOf(ctx->genericArgs()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    last_segment.span_ = node_span;
    last_segment.name_span_ = spanOf(ctx->pathIdentSegment());
    std::vector<PathSegment> path_segments;
    for (auto path_segment_ctx : ctx->typePathSegment())
    {
      path_segments.push_back(buildTypePathSegment(path_segment_ctx));
    }
    path_segments.push_back(std::move(last_segment));
    auto result = std::make_unique<PathType>(std::move(path_segments));
    result->span_ = spanOf(ctx);
    return result;
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
    std::unique_ptr<Item> item = buildItem(ctx_item);
    if (item)
    {
      items.push_back(std::move(item));
    }
  }
  auto result = std::make_unique<Crate>(std::move(items));
  result->span_ = spanOf(ctx);
  return result;
}

std::unique_ptr<Item> AstBuilder::buildItem(rx::RxParser::ItemContext *ctx)
{
  if (ctx->functionDefinition())
  {
    return buildFuncItem(ctx->functionDefinition());
  }
  else if (ctx->constantItem())
  {
    return buildConstItem(ctx->constantItem());
  }
  else if (ctx->inherentImpl())
  {
    return buildImplItem(ctx->inherentImpl());
  }
  else if (ctx->structDefinition())
  {
    return buildStructItem(ctx->structDefinition());
  }
  else
  {
    return nullptr;
  }
}

FuncParam AstBuilder::getFuncParam(rx::RxParser::FunctionParamContext *ctx)
{
  if (ctx->identifierBinding()->MUT())

  {
    auto result = FuncParam(ctx->identifierBinding()->identifier()->getText(),
                            true, buildType(ctx->typeRef()));
    result.span_ = spanOf(ctx);
    result.ident_span_ = spanOf(ctx->identifierBinding()->identifier());
    return result;
  }
  else
  {
    auto result = FuncParam(ctx->identifierBinding()->identifier()->getText(),
                            false, buildType(ctx->typeRef()));
    result.span_ = spanOf(ctx);
    result.ident_span_ = spanOf(ctx->identifierBinding()->identifier());
    return result;
  }
}

SelfParam AstBuilder::getSelfParam(rx::RxParser::SelfParamContext *ctx)
{
  auto result = SelfParam(bool(ctx->AMP()), bool(ctx->MUT()));
  result.span_ = spanOf(ctx);
  result.self_span_ = spanOf(ctx->SELF_VALUE()->getSymbol());
  return result;
}

std::unique_ptr<FuncItem>
AstBuilder::buildFuncItem(rx::RxParser::FunctionDefinitionContext *ctx)
{
  std::string ident = ctx->identifier()->getText();
  std::unique_ptr<BlockExpr> block_expr =
      std::move(buildBlockExpr(ctx->blockExpression()));
  std::optional<SelfParam> self_param = std::nullopt;
  std::vector<FuncParam> func_params;
  std::unique_ptr<Type> return_type;
  if (ctx->functionParameters())
  {
    for (auto func_param_ctx : ctx->functionParameters()->functionParam())
    {
      func_params.push_back(getFuncParam(func_param_ctx));
    }
    if (ctx->functionParameters()->selfParam())
    {
      self_param = getSelfParam(ctx->functionParameters()->selfParam());
    }
  }
  if (ctx->typeRef())
  {
    return_type = buildType(ctx->typeRef());
  }
  auto result = std::make_unique<FuncItem>(
      std::move(ident), self_param, std::move(func_params),
      std::move(return_type), std::move(block_expr));
  result->span_ = spanOf(ctx);
  result->ident_span_ = spanOf(ctx->identifier());
  return result;
}

StructField AstBuilder::gerStructField(rx::RxParser::StructFieldContext *ctx)
{
  auto result =
      StructField(ctx->identifier()->getText(), buildType(ctx->typeRef()));
  result.span_ = spanOf(ctx);
  result.ident_span_ = spanOf(ctx->identifier());
  return result;
}
std::vector<DeriveKind> AstBuilder::getDeriveKinds(
    std::vector<rx::RxParser::OuterAttributeContext *> ctxs)
{
  std::vector<DeriveKind> result;
  for (auto ctx : ctxs)
  {
    for (auto derive_name_ctx : ctx->deriveName())
    {
      if (derive_name_ctx->COPY())
      {
        result.push_back(DeriveKind::Copy);
      }
      else if (derive_name_ctx->CLONE())
      {
        result.push_back(DeriveKind::Clone);
      }
      else if (derive_name_ctx->PARTIAL_EQ())
      {
        result.push_back(DeriveKind::PartialEq);
      }
      else
      {
        result.push_back(DeriveKind::Eq);
      }
    }
  }
  return result;
}

std::unique_ptr<StructItem>
AstBuilder::buildStructItem(rx::RxParser::StructDefinitionContext *ctx)
{
  std::vector<DeriveKind> derives;
  if (ctx->outerAttribute().size())
  {
    derives = getDeriveKinds(ctx->outerAttribute());
  }
  std::string ident = ctx->identifier()->getText();
  std::vector<StructField> fields;
  for (auto struct_field_ctx : ctx->structField())
  {
    fields.push_back(gerStructField(struct_field_ctx));
  }
  auto result = std::make_unique<StructItem>(
      std::move(ident), std::move(derives), std::move(fields));
  result->span_ = spanOf(ctx);
  result->ident_span_ = spanOf(ctx->identifier());
  return result;
}
std::unique_ptr<ConstItem>
AstBuilder::buildConstItem(rx::RxParser::ConstantItemContext *ctx)
{
  auto result = std::make_unique<ConstItem>(ctx->identifier()->getText(),
                                            buildType(ctx->typeRef()),
                                            buildConstValue(ctx->constValue()));
  result->span_ = spanOf(ctx);
  result->ident_span_ = spanOf(ctx->identifier());
  return result;
}
std::unique_ptr<ImplItem>
AstBuilder::buildImplItem(rx::RxParser::InherentImplContext *ctx)
{
  std::unique_ptr<Type> type = buildType(ctx->typeRef());
  std::vector<std::unique_ptr<Item>> items;
  for (auto item_ctx : ctx->associatedItem())
  {
    if (item_ctx->constantItem())
    {
      items.push_back(buildConstItem(item_ctx->constantItem()));
    }
    else
    {
      items.push_back(buildFuncItem(item_ctx->functionDefinition()));
    }
  }
  auto result = std::make_unique<ImplItem>(std::move(type), std::move(items));
  result->span_ = spanOf(ctx);
  return result;
}

std::unique_ptr<BlockExpr>
AstBuilder::buildBlockExpr(rx::RxParser::BlockExpressionContext *ctx)
{
  std::vector<rx::RxParser::StatementContext *> stmt_ctxs = ctx->statement();
  auto expr_stmt_ctx = ctx->statementExpression();
  std::vector<std::unique_ptr<Stmt>> stmts;
  std::unique_ptr<Expr> tail_stmt;
  for (auto &stmt_ctx : stmt_ctxs)
  {
    auto stmt = buildStmt(stmt_ctx);
    if (stmt)
    {
      stmts.push_back(std::move(stmt));
    }
  }
  if (expr_stmt_ctx)
  {
    tail_stmt = std::move(buildStmtExpr(ctx->statementExpression()));
  }
  auto result =
      std::make_unique<BlockExpr>(std::move(stmts), std::move(tail_stmt));
  result->span_ = spanOf(ctx);
  return result;
}

std::unique_ptr<Stmt> AstBuilder::buildStmt(rx::RxParser::StatementContext *ctx)
{
  if (ctx->letStatement()) // let statement
  {
    return buildLetStmt(ctx->letStatement());
  }
  else if (ctx->expressionWithBlock())
  {
    auto result = std::make_unique<ExprStmt>(
        buildExprWithBlock(ctx->expressionWithBlock()));
    result->span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->statementExpression())
  {
    auto result =
        std::make_unique<ExprStmt>(buildStmtExpr(ctx->statementExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  return nullptr;
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
  auto result = std::make_unique<LetStmt>(std::move(ident), mut,
                                          std::move(type), std::move(expr));
  result->span_ = spanOf(ctx);
  result->ident_span_ = spanOf(ctx->identifierBinding()->identifier());
  return result;
}

std::unique_ptr<Expr>
AstBuilder::buildExpr(rx::RxParser::ExpressionContext *ctx)
{
  return buildAssignExpr(ctx->assignmentExpression());
}

std::unique_ptr<ExprStmt>
AstBuilder::buildExprStmt(rx::RxParser::StatementExpressionContext *ctx)
{
  auto result = std::make_unique<ExprStmt>(buildStmtExpr(ctx));
  result->span_ = spanOf(ctx);
  return result;
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
    auto result = std::make_unique<AssignExpr>(
        getAssignOperator(ctx->assignmentOperator()),
        buildOrExpr(ctx->logicalOrExpression()), buildExpr(ctx->expression()));
    result->span_ = spanOf(ctx);
    return result;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->logicalAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalOr, std::move(result_ptr),
        buildAndExpr(ctx->logicalAndExpression()[i]));
    result_ptr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->comparisonExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalAnd, std::move(result_ptr),
        buildCompExpr(ctx->comparisonExpression()[i]));
    result_ptr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitOrExpression()[0]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        BinaryOperator::Less, buildBitOrExpr(ctx->closedBitOrExpression()),
        buildBitOrExpr(ctx->bitOrExpression()[0]));
    result->span_ = node_span;
    return result;
  }
  if (ctx->comparisonExceptLt())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitOrExpression()[1]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result =
        std::make_unique<BinaryExpr>(getCompOperator(ctx->comparisonExceptLt()),
                                     buildBitOrExpr(ctx->bitOrExpression()[0]),
                                     buildBitOrExpr(ctx->bitOrExpression()[1]));
    result->span_ = node_span;
    return result;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildBitXorExpr(ctx->bitXorExpression()[i]));
    result_ptr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildBitXorExpr(ctx->bitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedBitXorExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitOr, std::move(result_ptr),
      buildBitXorExpr(ctx->closedBitXorExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitXorExpr(rx::RxParser::BitXorExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildBitAndExpr(ctx->bitAndExpression()[0]);
  for (int i = 1; i < ctx->bitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildBitAndExpr(ctx->bitAndExpression()[i]));
    result_ptr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildBitAndExpr(ctx->bitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedBitAndExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitXor, std::move(result_ptr),
      buildBitAndExpr(ctx->closedBitAndExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr>
AstBuilder::buildBitAndExpr(rx::RxParser::BitAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr = buildShiftExpr(ctx->shiftExpression()[0]);
  for (int i = 1; i < ctx->shiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->shiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildShiftExpr(ctx->shiftExpression()[i]));
    result_ptr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->shiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildShiftExpr(ctx->shiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedShiftExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitAnd, std::move(result_ptr),
      buildShiftExpr(ctx->closedShiftExpression()));
  result_ptr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }

  BinaryOperator last_op = getAddOperator(operators.back());
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->closedMultiplicativeExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result = std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildMultExpr(ctx->closedMultiplicativeExpression()));
  result->span_ = node_span;
  return result;
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
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(ctx->castExpression()[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<BinaryExpr>(
          getMultOperator(ctx->multiplicativeOperator()[i - 1]),
          std::move(result_expr), buildCastExpr(ctx->castExpression()[i]));
      result_expr->span_ = node_span;
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
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(cast_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildCastExpr(cast_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  BinaryOperator last_op = getMultOperator(operators[operators.size() - 1]);
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedCastExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result =
      std::make_unique<BinaryExpr>(last_op, std::move(result_expr),
                                   buildCastExpr(ctx->closedCastExpression()));
  result->span_ = node_span;
  return result;
}
std::unique_ptr<Expr>
AstBuilder::buildCastExpr(rx::RxParser::CastExpressionContext *ctx)
{
  if (ctx->AS().size())
  {
    std::unique_ptr<Expr> result_ptr = buildUnaryExpr(ctx->unaryExpression());
    for (auto type_ctx : ctx->typeRef())
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(type_ctx->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_ptr = std::make_unique<CastExpr>(std::move(result_ptr),
                                              buildType(type_ctx));
      result_ptr->span_ = node_span;
    }
    return std::move(result_ptr);
  }
  return buildUnaryExpr(ctx->unaryExpression());
}
std::unique_ptr<Expr>
AstBuilder::buildCastExpr(rx::RxParser::ClosedCastExpressionContext *ctx)
{
  if (ctx->AS())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->closedCastType()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result =
        std::make_unique<CastExpr>(buildCastExpr(ctx->castExpression()),
                                   buildClosedCastType(ctx->closedCastType()));
    result->span_ = node_span;
    return result;
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
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Negate, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->NOT())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Not, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->STAR())
    {
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Dereference,
                                                std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->AMP())
    {
      UnaryOperator kind =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto result = std::make_unique<UnaryExpr>(kind, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      UnaryOperator kind_in =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto inner_span = spanOf(ctx);
      if (inner_span && inner_span->begin < inner_span->end)
      {
        ++inner_span->begin;
      }
      else
      {
        inner_span = std::nullopt;
      }
      auto inner_node = std::make_unique<UnaryExpr>(kind_in, std::move(expr));
      inner_node->span_ = inner_span;
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Borrow,
                                                std::move(inner_node));
      result->span_ = spanOf(ctx);
      return result;
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
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<CallExpr>(std::move(result_expr),
                                               std::move(arguments));
      result_expr->span_ = node_span;
    }
    else if (postfixs[i]->expression())
    {
      std::unique_ptr<Expr> index_expr = buildExpr(postfixs[i]->expression());
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<IndexExpr>(std::move(result_expr),
                                                std::move(index_expr));
      result_expr->span_ = node_span;
    }
    else
    {
      result_expr =
          buildDotSuffix(std::move(result_expr), postfixs[i]->dotSuffix());
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr->span_ = node_span;
    }
  }
  return result_expr;
}

std::unique_ptr<Expr>
AstBuilder::buildDotSuffix(std::unique_ptr<Expr> base,
                           rx::RxParser::DotSuffixContext *ctx)
{
  const auto suffix_span = spanOf(ctx);
  const auto full_span =
      base->span_ && suffix_span
          ? std::optional<SourceSpan>(cover(*base->span_, *suffix_span))
          : std::nullopt;
  if (ctx->callArguments())
  {
    std::vector<std::unique_ptr<Expr>> arguments;
    for (auto expr_ctx : ctx->callArguments()->expression())
    {
      arguments.push_back(buildExpr(expr_ctx));
    }
    auto result = std::make_unique<MethodCallExpr>(
        std::move(base), buildPathExprSegment(ctx->pathExprSegment()),
        std::move(arguments));
    result->span_ = full_span;
    return result;
  }
  else
  {
    auto result = std::make_unique<MemberExpr>(std::move(base),
                                               ctx->identifier()->getText());
    result->span_ = full_span;
    result->member_span_ = spanOf(ctx->identifier());
    return result;
  }
}

std::unique_ptr<Expr>
AstBuilder::buildPrimaryExpr(rx::RxParser::PrimaryExpressionContext *ctx)
{
  if (ctx->nonBlockPrimary())
  {
    return buildNonBlockPrimary(ctx->nonBlockPrimary());
  }
  else
  {
    return buildExprWithBlock(ctx->expressionWithBlock());
  }
}

std::unique_ptr<Expr>
AstBuilder::buildNonBlockPrimary(rx::RxParser::NonBlockPrimaryContext *ctx)
{
  if (ctx->literalExpression())
  {
    return buildLiteralExpr(ctx->literalExpression());
  }
  else if (ctx->pathInExpression())
  {
    if (!ctx->LBRACE())
    {
      return buildPathExpr(ctx->pathInExpression());
    }
    else
    {
      auto result = std::make_unique<StructExpr>(
          buildPathExpr(ctx->pathInExpression()),
          buildStructExprFields(ctx->structExprFields()));
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else if (ctx->LPAREN())
  {
    if (!ctx->expression())
    {
      auto result = std::make_unique<UnitExpr>();
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      return buildExpr(ctx->expression());
    }
  }
  else if (ctx->arrayExpression())
  {
    auto array_ctx = ctx->arrayExpression();
    if (array_ctx->SEMI())
    {
      auto result =
          std::make_unique<ArrayExpr>(buildExpr(array_ctx->expression(0)),
                                      buildConstValue(array_ctx->constValue()));
      result->span_ = spanOf(ctx);
      return result;
    }
    std::vector<std::unique_ptr<Expr>> exprs;
    for (auto expr_ctx : array_ctx->expression())
    {
      exprs.push_back(buildExpr(expr_ctx));
    }
    auto result = std::make_unique<ArrayExpr>(std::move(exprs));
    result->span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->BREAK())
  {
    if (ctx->expression())
    {
      auto result = std::make_unique<BreakExpr>(buildExpr(ctx->expression()));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      auto result = std::make_unique<BreakExpr>(nullptr);
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else if (ctx->RETURN())
  {
    if (ctx->expression())
    {
      auto result = std::make_unique<ReturnExpr>(buildExpr(ctx->expression()));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      auto result = std::make_unique<ReturnExpr>(nullptr);
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    auto result = std::make_unique<ContinueExpr>();
    result->span_ = spanOf(ctx);
    return result;
  }
}

std::vector<StructExprField>
AstBuilder::buildStructExprFields(rx::RxParser::StructExprFieldsContext *ctx)
{
  std::vector<StructExprField> result{};
  if (!ctx)
  {
    return result;
  }
  for (auto field : ctx->structExprField())
  {
    result.push_back(buildStructExprField(field));
  }
  return result;
}

StructExprField
AstBuilder::buildStructExprField(rx::RxParser::StructExprFieldContext *ctx)
{
  auto result = StructExprField(ctx->identifier()->getText(),
                                buildExpr(ctx->expression()));
  result.span_ = spanOf(ctx);
  result.name_span_ = spanOf(ctx->identifier());
  return result;
}

std::unique_ptr<LiteralExpr>
AstBuilder::buildLiteralExpr(rx::RxParser::LiteralExpressionContext *ctx)
{
  if (ctx->INTEGER_LITERAL())
  {
    auto result = std::make_unique<LiteralExpr>(
        true, true, LiteralStringToInt(ctx->INTEGER_LITERAL()->getText()),
        LiteralStringToIntegerType(ctx->INTEGER_LITERAL()->getText()));
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->FALSE())
  {
    auto result =
        std::make_unique<LiteralExpr>(false, false, 0, IntegerType::Inferred);
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
  }
  else
  {
    auto result =
        std::make_unique<LiteralExpr>(false, true, 0, IntegerType::Inferred);
    result->span_ = spanOf(ctx);
    result->literal_span_ = spanOf(ctx);
    return result;
  }
}

std::unique_ptr<Expr>
AstBuilder::buildConditionExpr(rx::RxParser::ConditionExpressionContext *ctx)
{
  return buildConditionAssignExpr(ctx->conditionAssignmentExpression());
}

std::unique_ptr<Expr> AstBuilder::buildConditionAssignExpr(
    rx::RxParser::ConditionAssignmentExpressionContext *ctx)
{
  if (ctx->assignmentOperator())
  {
    auto result = std::make_unique<AssignExpr>(
        getAssignOperator(ctx->assignmentOperator()),
        buildConditionOrExpr(ctx->conditionLogicalOrExpression()),
        buildConditionExpr(ctx->conditionExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  return buildConditionOrExpr(ctx->conditionLogicalOrExpression());
}

std::unique_ptr<Expr> AstBuilder::buildConditionOrExpr(
    rx::RxParser::ConditionLogicalOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionAndExpr(ctx->conditionLogicalAndExpression()[0]);
  for (int i = 1; i < ctx->conditionLogicalAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionLogicalAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalOr, std::move(result_ptr),
        buildConditionAndExpr(ctx->conditionLogicalAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionAndExpr(
    rx::RxParser::ConditionLogicalAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionCompExpr(ctx->conditionComparisonExpression()[0]);
  for (int i = 1; i < ctx->conditionComparisonExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionComparisonExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalAnd, std::move(result_ptr),
        buildConditionCompExpr(ctx->conditionComparisonExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionCompExpr(
    rx::RxParser::ConditionComparisonExpressionContext *ctx)
{
  if (ctx->LT())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitOrExpression()[0]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        BinaryOperator::Less,
        buildConditionBitOrExpr(ctx->conditionClosedBitOrExpression()),
        buildConditionBitOrExpr(ctx->conditionBitOrExpression()[0]));
    result->span_ = node_span;
    return result;
  }
  if (ctx->comparisonExceptLt())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitOrExpression()[1]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        getCompOperator(ctx->comparisonExceptLt()),
        buildConditionBitOrExpr(ctx->conditionBitOrExpression()[0]),
        buildConditionBitOrExpr(ctx->conditionBitOrExpression()[1]));
    result->span_ = node_span;
    return result;
  }
  return buildConditionBitOrExpr(ctx->conditionBitOrExpression()[0]);
}

std::unique_ptr<Expr> AstBuilder::buildConditionBitOrExpr(
    rx::RxParser::ConditionBitOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBitXorExpr(ctx->conditionBitXorExpression()[0]);
  for (int i = 1; i < ctx->conditionBitXorExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildConditionBitXorExpr(ctx->conditionBitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionBitOrExpr(
    rx::RxParser::ConditionClosedBitOrExpressionContext *ctx)
{
  if (ctx->PIPE().size() == 0)
  {
    return buildConditionBitXorExpr(ctx->conditionClosedBitXorExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildConditionBitXorExpr(ctx->conditionBitXorExpression()[0]);
  for (int i = 1; i < ctx->conditionBitXorExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildConditionBitXorExpr(ctx->conditionBitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedBitXorExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitOr, std::move(result_ptr),
      buildConditionBitXorExpr(ctx->conditionClosedBitXorExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionBitXorExpr(
    rx::RxParser::ConditionBitXorExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBitAndExpr(ctx->conditionBitAndExpression()[0]);
  for (int i = 1; i < ctx->conditionBitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildConditionBitAndExpr(ctx->conditionBitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionBitXorExpr(
    rx::RxParser::ConditionClosedBitXorExpressionContext *ctx)
{
  if (ctx->CARET().size() == 0)
  {
    return buildConditionBitAndExpr(ctx->conditionClosedBitAndExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildConditionBitAndExpr(ctx->conditionBitAndExpression()[0]);
  for (int i = 1; i < ctx->conditionBitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildConditionBitAndExpr(ctx->conditionBitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedBitAndExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitXor, std::move(result_ptr),
      buildConditionBitAndExpr(ctx->conditionClosedBitAndExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionBitAndExpr(
    rx::RxParser::ConditionBitAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionShiftExpr(ctx->conditionShiftExpression()[0]);
  for (int i = 1; i < ctx->conditionShiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionShiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildConditionShiftExpr(ctx->conditionShiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionBitAndExpr(
    rx::RxParser::ConditionClosedBitAndExpressionContext *ctx)
{
  if (ctx->AMP().size() == 0)
  {
    return buildConditionShiftExpr(ctx->conditionClosedShiftExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildConditionShiftExpr(ctx->conditionShiftExpression()[0]);
  for (int i = 1; i < ctx->conditionShiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionShiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildConditionShiftExpr(ctx->conditionShiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedShiftExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitAnd, std::move(result_ptr),
      buildConditionShiftExpr(ctx->conditionClosedShiftExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionShiftExpr(
    rx::RxParser::ConditionShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
            child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::ConditionClosedAdditiveExpressionContext
                              *>(child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::ConditionClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}

std::unique_ptr<Expr> AstBuilder::buildConditionShiftExpr(
    rx::RxParser::ConditionClosedShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
            child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::ConditionClosedAdditiveExpressionContext
                              *>(child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::ConditionClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}

std::unique_ptr<Expr> AstBuilder::buildConditionAddExpr(
    rx::RxParser::ConditionAdditiveExpressionContext *ctx)
{
  auto mult_expr_contexts = ctx->conditionMultiplicativeExpression();
  auto operators = ctx->additiveOperator();

  std::unique_ptr<Expr> result_expr =
      buildConditionMultExpr(mult_expr_contexts[0]);
  for (int i = 1; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i - 1]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr),
        buildConditionMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  return result_expr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionAddExpr(
    rx::RxParser::ConditionClosedAdditiveExpressionContext *ctx)
{
  auto mult_expr_contexts = ctx->conditionMultiplicativeExpression();
  auto operators = ctx->additiveOperator();
  if (!operators.size())
  {
    return buildConditionMultExpr(
        ctx->conditionClosedMultiplicativeExpression());
  }
  std::unique_ptr<Expr> result_expr =
      buildConditionMultExpr(mult_expr_contexts[0]);
  for (int i = 1; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i - 1]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr),
        buildConditionMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }

  BinaryOperator last_op = getAddOperator(operators.back());
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedMultiplicativeExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result = std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildConditionMultExpr(ctx->conditionClosedMultiplicativeExpression()));
  result->span_ = node_span;
  return result;
}

std::unique_ptr<Expr> AstBuilder::buildConditionMultExpr(
    rx::RxParser::ConditionMultiplicativeExpressionContext *ctx)
{
  if (ctx->multiplicativeOperator().size())
  {
    std::unique_ptr<Expr> result_expr =
        buildConditionCastExpr(ctx->conditionCastExpression()[0]);
    for (int i = 1; i < ctx->conditionCastExpression().size(); i++)
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last =
          spanOf(ctx->conditionCastExpression()[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<BinaryExpr>(
          getMultOperator(ctx->multiplicativeOperator()[i - 1]),
          std::move(result_expr),
          buildConditionCastExpr(ctx->conditionCastExpression()[i]));
      result_expr->span_ = node_span;
    }
    return std::move(result_expr);
  }
  return buildConditionCastExpr(ctx->conditionCastExpression()[0]);
}

std::unique_ptr<Expr> AstBuilder::buildConditionMultExpr(
    rx::RxParser::ConditionClosedMultiplicativeExpressionContext *ctx)
{
  auto cast_expr_contexts = ctx->conditionCastExpression();
  auto operators = ctx->multiplicativeOperator();
  if (!operators.size())
  {
    return buildConditionCastExpr(ctx->conditionClosedCastExpression());
  }
  std::unique_ptr<Expr> result_expr =
      buildConditionCastExpr(cast_expr_contexts[0]);
  for (int i = 1; i < cast_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getMultOperator(operators[i - 1]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(cast_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr),
        buildConditionCastExpr(cast_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  BinaryOperator last_op = getMultOperator(operators[operators.size() - 1]);
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedCastExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result = std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildConditionCastExpr(ctx->conditionClosedCastExpression()));
  result->span_ = node_span;
  return result;
}

std::unique_ptr<Expr> AstBuilder::buildConditionCastExpr(
    rx::RxParser::ConditionCastExpressionContext *ctx)
{
  if (ctx->AS().size())
  {
    std::unique_ptr<Expr> result_ptr =
        buildConditionUnaryExpr(ctx->conditionUnaryExpression());
    for (auto type_ctx : ctx->typeRef())
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(type_ctx->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_ptr = std::make_unique<CastExpr>(std::move(result_ptr),
                                              buildType(type_ctx));
      result_ptr->span_ = node_span;
    }
    return result_ptr;
  }
  return buildConditionUnaryExpr(ctx->conditionUnaryExpression());
}

std::unique_ptr<Expr> AstBuilder::buildConditionCastExpr(
    rx::RxParser::ConditionClosedCastExpressionContext *ctx)
{
  if (ctx->AS())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->closedCastType()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<CastExpr>(
        buildConditionCastExpr(ctx->conditionCastExpression()),
        buildClosedCastType(ctx->closedCastType()));
    result->span_ = node_span;
    return result;
  }
  return buildConditionUnaryExpr(ctx->conditionUnaryExpression());
}

std::unique_ptr<Expr> AstBuilder::buildConditionUnaryExpr(
    rx::RxParser::ConditionUnaryExpressionContext *ctx)
{
  if (ctx->unaryOperator())
  {
    auto op = ctx->unaryOperator();
    auto expr = buildConditionUnaryExpr(ctx->conditionUnaryExpression());
    if (op->MINUS())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Negate, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->NOT())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Not, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->STAR())
    {
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Dereference,
                                                std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->AMP())
    {
      UnaryOperator kind =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto result = std::make_unique<UnaryExpr>(kind, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      UnaryOperator kind_in =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto inner_span = spanOf(ctx);
      if (inner_span && inner_span->begin < inner_span->end)
      {
        ++inner_span->begin;
      }
      else
      {
        inner_span = std::nullopt;
      }
      auto inner_node = std::make_unique<UnaryExpr>(kind_in, std::move(expr));
      inner_node->span_ = inner_span;
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Borrow,
                                                std::move(inner_node));
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    return buildConditionPostfixExpr(ctx->conditionPostfixExpression());
  }
}

std::unique_ptr<Expr> AstBuilder::buildConditionPostfixExpr(
    rx::RxParser::ConditionPostfixExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_expr =
      buildConditionPrimaryExpr(ctx->conditionPrimary());
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
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<CallExpr>(std::move(result_expr),
                                               std::move(arguments));
      result_expr->span_ = node_span;
    }
    else if (postfixs[i]->expression())
    {
      std::unique_ptr<Expr> index_expr = buildExpr(postfixs[i]->expression());
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<IndexExpr>(std::move(result_expr),
                                                std::move(index_expr));
      result_expr->span_ = node_span;
    }
    else
    {
      result_expr =
          buildDotSuffix(std::move(result_expr), postfixs[i]->dotSuffix());
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr->span_ = node_span;
    }
  }
  return result_expr;
}

std::unique_ptr<Expr> AstBuilder::buildConditionPrimaryExpr(
    rx::RxParser::ConditionPrimaryContext *ctx)
{
  if (ctx->conditionPrimaryWithoutBareBlock())
  {
    return buildConditionPrimaryExprWithoutBareBlock(
        ctx->conditionPrimaryWithoutBareBlock());
  }
  else
  {
    return buildBlockExpr(ctx->blockExpression());
  }
}

std::unique_ptr<Expr> AstBuilder::buildConditionPrimaryExprWithoutBareBlock(
    rx::RxParser::ConditionPrimaryWithoutBareBlockContext *ctx)
{
  if (ctx->literalExpression())
  {
    return buildLiteralExpr(ctx->literalExpression());
  }
  else if (ctx->pathInExpression())
  {
    return buildPathExpr(ctx->pathInExpression());
  }
  else if (ctx->LPAREN())
  {
    if (ctx->expression())
    {
      return buildExpr(ctx->expression());
    }
    else
    {
      auto result = std::make_unique<UnitExpr>();
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else if (ctx->arrayExpression())
  {
    return buildArrayExpr(ctx->arrayExpression());
  }
  else if (ctx->ifExpression())
  {
    return buildIfExpr(ctx->ifExpression());
  }
  else if (ctx->LOOP())
  {
    auto result =
        std::make_unique<LoopExpr>(buildBlockExpr(ctx->blockExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->WHILE())
  {
    auto result = std::make_unique<WhileExpr>(
        buildConditionExpr(ctx->conditionExpression()),
        buildBlockExpr(ctx->blockExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->BREAK())
  {
    if (ctx->conditionBreakExpression())
    {
      auto result = std::make_unique<BreakExpr>(
          buildConditionBreakExpr(ctx->conditionBreakExpression()));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      auto result = std::make_unique<BreakExpr>(nullptr);
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else if (ctx->RETURN())
  {
    if (ctx->conditionExpression())
    {
      auto result = std::make_unique<ReturnExpr>(
          buildConditionExpr(ctx->conditionExpression()));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      auto result = std::make_unique<ReturnExpr>(nullptr);
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    auto result = std::make_unique<ContinueExpr>();
    result->span_ = spanOf(ctx);
    return result;
  }
}

std::unique_ptr<ArrayExpr>
AstBuilder::buildArrayExpr(rx::RxParser::ArrayExpressionContext *ctx)
{
  if (ctx->expression().size() != 0)
  {
    if (ctx->SEMI())
    {
      auto result = std::make_unique<ArrayExpr>(
          buildExpr(ctx->expression()[0]), buildConstValue(ctx->constValue()));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      std::vector<std::unique_ptr<Expr>> exprs;
      for (auto expr_ctx : ctx->expression())
      {
        exprs.push_back(std::move(buildExpr(expr_ctx)));
      }
      auto result = std::make_unique<ArrayExpr>(std::move(exprs));
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  auto result =
      std::make_unique<ArrayExpr>(std::vector<std::unique_ptr<Expr>>());
  result->span_ = spanOf(ctx);
  return result;
}
std::unique_ptr<IfExpr>
AstBuilder::buildIfExpr(rx::RxParser::IfExpressionContext *ctx)
{
  std::unique_ptr condition_ptr =
      buildConditionExpr(ctx->conditionExpression());
  std::unique_ptr block_ptr = buildBlockExpr(ctx->blockExpression()[0]);
  if (ctx->ELSE())
  {
    if (ctx->blockExpression().size() == 2)
    {
      auto result = std::make_unique<IfExpr>(
          std::move(condition_ptr), std::move(block_ptr), true, false,
          buildBlockExpr(ctx->blockExpression()[1]), nullptr);
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      auto result = std::make_unique<IfExpr>(
          std::move(condition_ptr), std::move(block_ptr), true, true, nullptr,
          buildIfExpr(ctx->ifExpression()));
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    auto result = std::make_unique<IfExpr>(std::move(condition_ptr),
                                           std::move(block_ptr));
    result->span_ = spanOf(ctx);
    return result;
  }
}

std::unique_ptr<Expr> AstBuilder::buildConditionBreakExpr(
    rx::RxParser::ConditionBreakExpressionContext *ctx)
{
  return buildConditionBreakAssignExpr(
      ctx->conditionBreakAssignmentExpression());
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakAssignExpr(
    rx::RxParser::ConditionBreakAssignmentExpressionContext *ctx)
{
  if (ctx->assignmentOperator())
  {
    auto result = std::make_unique<AssignExpr>(
        getAssignOperator(ctx->assignmentOperator()),
        buildConditionBreakOrExpr(ctx->conditionBreakLogicalOrExpression()),
        buildConditionExpr(ctx->conditionExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  return buildConditionBreakOrExpr(ctx->conditionBreakLogicalOrExpression());
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakOrExpr(
    rx::RxParser::ConditionBreakLogicalOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakAndExpr(ctx->conditionBreakLogicalAndExpression());
  for (int i = 0; i < ctx->conditionLogicalAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionLogicalAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalOr, std::move(result_ptr),
        buildConditionAndExpr(ctx->conditionLogicalAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakAndExpr(
    rx::RxParser::ConditionBreakLogicalAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakCompExpr(ctx->conditionBreakComparisonExpression());
  for (int i = 0; i < ctx->conditionComparisonExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionComparisonExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalAnd, std::move(result_ptr),
        buildConditionCompExpr(ctx->conditionComparisonExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakCompExpr(
    rx::RxParser::ConditionBreakComparisonExpressionContext *ctx)
{
  if (ctx->LT())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitOrExpression()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        BinaryOperator::Less,
        buildConditionBreakBitOrExpr(
            ctx->conditionBreakClosedBitOrExpression()),
        buildConditionBitOrExpr(ctx->conditionBitOrExpression()));
    result->span_ = node_span;
    return result;
  }
  if (ctx->comparisonExceptLt())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitOrExpression()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        getCompOperator(ctx->comparisonExceptLt()),
        buildConditionBreakBitOrExpr(ctx->conditionBreakBitOrExpression()),
        buildConditionBitOrExpr(ctx->conditionBitOrExpression()));
    result->span_ = node_span;
    return result;
  }
  return buildConditionBreakBitOrExpr(ctx->conditionBreakBitOrExpression());
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakBitOrExpr(
    rx::RxParser::ConditionBreakBitOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakBitXorExpr(ctx->conditionBreakBitXorExpression());
  for (int i = 0; i < ctx->conditionBitXorExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildConditionBitXorExpr(ctx->conditionBitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakBitOrExpr(
    rx::RxParser::ConditionBreakClosedBitOrExpressionContext *ctx)
{
  if (ctx->PIPE().size() == 0)
  {
    return buildConditionBreakBitXorExpr(
        ctx->conditionBreakClosedBitXorExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakBitXorExpr(ctx->conditionBreakBitXorExpression());
  for (int i = 0; i < ctx->conditionBitXorExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildConditionBitXorExpr(ctx->conditionBitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedBitXorExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitOr, std::move(result_ptr),
      buildConditionBitXorExpr(ctx->conditionClosedBitXorExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakBitXorExpr(
    rx::RxParser::ConditionBreakBitXorExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakBitAndExpr(ctx->conditionBreakBitAndExpression());
  for (int i = 0; i < ctx->conditionBitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildConditionBitAndExpr(ctx->conditionBitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakBitXorExpr(
    rx::RxParser::ConditionBreakClosedBitXorExpressionContext *ctx)
{
  if (ctx->CARET().size() == 0)
  {
    return buildConditionBreakBitAndExpr(
        ctx->conditionBreakClosedBitAndExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakBitAndExpr(ctx->conditionBreakBitAndExpression());
  for (int i = 0; i < ctx->conditionBitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionBitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildConditionBitAndExpr(ctx->conditionBitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedBitAndExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitXor, std::move(result_ptr),
      buildConditionBitAndExpr(ctx->conditionClosedBitAndExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakBitAndExpr(
    rx::RxParser::ConditionBreakBitAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakShiftExpr(ctx->conditionBreakShiftExpression());
  for (int i = 0; i < ctx->conditionShiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionShiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildConditionShiftExpr(ctx->conditionShiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakBitAndExpr(
    rx::RxParser::ConditionBreakClosedBitAndExpressionContext *ctx)
{
  if (ctx->AMP().size() == 0)
  {
    return buildConditionBreakShiftExpr(
        ctx->conditionBreakClosedShiftExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildConditionBreakShiftExpr(ctx->conditionBreakShiftExpression());
  for (int i = 0; i < ctx->conditionShiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->conditionShiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildConditionShiftExpr(ctx->conditionShiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedShiftExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitAnd, std::move(result_ptr),
      buildConditionShiftExpr(ctx->conditionClosedShiftExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakShiftExpr(
    rx::RxParser::ConditionBreakShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::ConditionBreakAdditiveExpressionContext *>(
            child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ConditionBreakAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildConditionBreakAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<
                 rx::RxParser::ConditionBreakClosedAdditiveExpressionContext *>(
                 child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::ConditionBreakClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildConditionBreakAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
    else if (dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
                 child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::ConditionClosedAdditiveExpressionContext
                              *>(child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::ConditionClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakShiftExpr(
    rx::RxParser::ConditionBreakClosedShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::ConditionBreakAdditiveExpressionContext *>(
            child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ConditionBreakAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildConditionBreakAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<
                 rx::RxParser::ConditionBreakClosedAdditiveExpressionContext *>(
                 child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::ConditionBreakClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildConditionBreakAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
    else if (dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
                 child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::ConditionAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::ConditionClosedAdditiveExpressionContext
                              *>(child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::ConditionClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildConditionAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakAddExpr(
    rx::RxParser::ConditionBreakAdditiveExpressionContext *ctx)
{
  auto mult_expr_context = ctx->conditionBreakMultiplicativeExpression();
  auto mult_expr_contexts = ctx->conditionMultiplicativeExpression();
  auto operators = ctx->additiveOperator();

  std::unique_ptr<Expr> result_expr =
      buildConditionBreakMultExpr(mult_expr_context);
  for (int i = 0; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr),
        buildConditionMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  return result_expr;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakAddExpr(
    rx::RxParser::ConditionBreakClosedAdditiveExpressionContext *ctx)
{
  auto mult_expr_contexts = ctx->conditionMultiplicativeExpression();
  auto operators = ctx->additiveOperator();
  if (!operators.size())
  {
    return buildConditionBreakMultExpr(
        ctx->conditionBreakClosedMultiplicativeExpression());
  }
  std::unique_ptr<Expr> result_expr = buildConditionBreakMultExpr(
      ctx->conditionBreakMultiplicativeExpression());
  for (int i = 0; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr),
        buildConditionMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }

  BinaryOperator last_op = getAddOperator(operators.back());
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedMultiplicativeExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result = std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildConditionMultExpr(ctx->conditionClosedMultiplicativeExpression()));
  result->span_ = node_span;
  return result;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakMultExpr(
    rx::RxParser::ConditionBreakMultiplicativeExpressionContext *ctx)
{
  if (ctx->multiplicativeOperator().size())
  {
    std::unique_ptr<Expr> result_expr =
        buildConditionBreakCastExpr(ctx->conditionBreakCastExpression());
    for (int i = 0; i < ctx->conditionCastExpression().size(); i++)
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last =
          spanOf(ctx->conditionCastExpression()[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<BinaryExpr>(
          getMultOperator(ctx->multiplicativeOperator()[i]),
          std::move(result_expr),
          buildConditionCastExpr(ctx->conditionCastExpression()[i]));
      result_expr->span_ = node_span;
    }
    return std::move(result_expr);
  }
  return buildConditionBreakCastExpr(ctx->conditionBreakCastExpression());
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakMultExpr(
    rx::RxParser::ConditionBreakClosedMultiplicativeExpressionContext *ctx)
{
  auto cast_expr_contexts = ctx->conditionCastExpression();
  auto operators = ctx->multiplicativeOperator();
  if (!operators.size())
  {
    return buildConditionBreakCastExpr(
        ctx->conditionBreakClosedCastExpression());
  }
  std::unique_ptr<Expr> result_expr =
      buildConditionBreakCastExpr(ctx->conditionBreakCastExpression());
  for (int i = 0; i < cast_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getMultOperator(operators[i]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(cast_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr),
        buildConditionCastExpr(cast_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  BinaryOperator last_op = getMultOperator(operators[operators.size() - 1]);
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->conditionClosedCastExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result = std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildConditionCastExpr(ctx->conditionClosedCastExpression()));
  result->span_ = node_span;
  return result;
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakCastExpr(
    rx::RxParser::ConditionBreakCastExpressionContext *ctx)
{
  if (ctx->AS().size())
  {
    std::unique_ptr<Expr> result_ptr =
        buildConditionBreakUnaryExpr(ctx->conditionBreakUnaryExpression());
    for (auto type_ctx : ctx->typeRef())
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(type_ctx->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_ptr = std::make_unique<CastExpr>(std::move(result_ptr),
                                              buildType(type_ctx));
      result_ptr->span_ = node_span;
    }
    return std::move(result_ptr);
  }
  return buildConditionBreakUnaryExpr(ctx->conditionBreakUnaryExpression());
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakCastExpr(
    rx::RxParser::ConditionBreakClosedCastExpressionContext *ctx)
{
  if (ctx->AS())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->closedCastType()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<CastExpr>(
        buildConditionBreakCastExpr(ctx->conditionBreakCastExpression()),
        buildClosedCastType(ctx->closedCastType()));
    result->span_ = node_span;
    return result;
  }
  return buildConditionBreakUnaryExpr(ctx->conditionBreakUnaryExpression());
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakUnaryExpr(
    rx::RxParser::ConditionBreakUnaryExpressionContext *ctx)
{
  if (ctx->unaryOperator())
  {
    auto op = ctx->unaryOperator();
    auto expr = buildConditionUnaryExpr(ctx->conditionUnaryExpression());
    if (op->MINUS())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Negate, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->NOT())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Not, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->STAR())
    {
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Dereference,
                                                std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->AMP())
    {
      UnaryOperator kind =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto result = std::make_unique<UnaryExpr>(kind, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      UnaryOperator kind_in =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto inner_span = spanOf(ctx);
      if (inner_span && inner_span->begin < inner_span->end)
      {
        ++inner_span->begin;
      }
      else
      {
        inner_span = std::nullopt;
      }
      auto inner_node = std::make_unique<UnaryExpr>(kind_in, std::move(expr));
      inner_node->span_ = inner_span;
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Borrow,
                                                std::move(inner_node));
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    return buildConditionBreakPostfixExpr(
        ctx->conditionBreakPostfixExpression());
  }
}
std::unique_ptr<Expr> AstBuilder::buildConditionBreakPostfixExpr(
    rx::RxParser::ConditionBreakPostfixExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_expr = buildConditionPrimaryExprWithoutBareBlock(
      ctx->conditionPrimaryWithoutBareBlock());
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
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<CallExpr>(std::move(result_expr),
                                               std::move(arguments));
      result_expr->span_ = node_span;
    }
    else if (postfixs[i]->expression())
    {
      std::unique_ptr<Expr> index_expr = buildExpr(postfixs[i]->expression());
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<IndexExpr>(std::move(result_expr),
                                                std::move(index_expr));
      result_expr->span_ = node_span;
    }
    else
    {
      result_expr =
          buildDotSuffix(std::move(result_expr), postfixs[i]->dotSuffix());
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(postfixs[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr->span_ = node_span;
    }
  }
  return result_expr;
}

std::unique_ptr<Expr>
AstBuilder::buildStmtExpr(rx::RxParser::StatementExpressionContext *ctx)
{
  return buildStmtAssignExpr(ctx->statementAssignmentExpression());
}
std::unique_ptr<Expr> AstBuilder::buildStmtAssignExpr(
    rx::RxParser::StatementAssignmentExpressionContext *ctx)
{
  if (ctx->assignmentOperator())
  {
    auto result = std::make_unique<AssignExpr>(
        getAssignOperator(ctx->assignmentOperator()),
        buildStmtOrExpr(ctx->statementLogicalOrExpression()),
        buildExpr(ctx->expression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  return buildStmtOrExpr(ctx->statementLogicalOrExpression());
}
std::unique_ptr<Expr> AstBuilder::buildStmtOrExpr(
    rx::RxParser::StatementLogicalOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildStmtAndExpr(ctx->statementLogicalAndExpression());
  for (int i = 0; i < ctx->logicalAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->logicalAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalOr, std::move(result_ptr),
        buildAndExpr(ctx->logicalAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtAndExpr(
    rx::RxParser::StatementLogicalAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildStmtCompExpr(ctx->statementComparisonExpression());
  for (int i = 0; i < ctx->comparisonExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last =
        spanOf(ctx->comparisonExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::LogicalAnd, std::move(result_ptr),
        buildCompExpr(ctx->comparisonExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtCompExpr(
    rx::RxParser::StatementComparisonExpressionContext *ctx)
{
  if (ctx->LT())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitOrExpression()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        BinaryOperator::Less,
        buildStmtBitOrExpr(ctx->statementClosedBitOrExpression()),
        buildBitOrExpr(ctx->bitOrExpression()));
    result->span_ = node_span;
    return result;
  }
  if (ctx->comparisonExceptLt())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitOrExpression()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<BinaryExpr>(
        getCompOperator(ctx->comparisonExceptLt()),
        buildStmtBitOrExpr(ctx->statementBitOrExpression()),
        buildBitOrExpr(ctx->bitOrExpression()));
    result->span_ = node_span;
    return result;
  }
  return buildStmtBitOrExpr(ctx->statementBitOrExpression());
}
std::unique_ptr<Expr> AstBuilder::buildStmtBitOrExpr(
    rx::RxParser::StatementBitOrExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildStmtBitXorExpr(ctx->statementBitXorExpression());
  for (int i = 0; i < ctx->bitXorExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildBitXorExpr(ctx->bitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtBitOrExpr(
    rx::RxParser::StatementClosedBitOrExpressionContext *ctx)
{
  if (ctx->PIPE().size() == 0)
  {
    return buildStmtBitXorExpr(ctx->statementClosedBitXorExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildStmtBitXorExpr(ctx->statementBitXorExpression());
  for (int i = 0; i < ctx->bitXorExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitXorExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitOr, std::move(result_ptr),
        buildBitXorExpr(ctx->bitXorExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedBitXorExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitOr, std::move(result_ptr),
      buildBitXorExpr(ctx->closedBitXorExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtBitXorExpr(
    rx::RxParser::StatementBitXorExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildStmtBitAndExpr(ctx->statementBitAndExpression());
  for (int i = 0; i < ctx->bitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildBitAndExpr(ctx->bitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtBitXorExpr(
    rx::RxParser::StatementClosedBitXorExpressionContext *ctx)
{
  if (ctx->CARET().size() == 0)
  {
    return buildStmtBitAndExpr(ctx->statementClosedBitAndExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildStmtBitAndExpr(ctx->statementBitAndExpression());
  for (int i = 0; i < ctx->bitAndExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->bitAndExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitXor, std::move(result_ptr),
        buildBitAndExpr(ctx->bitAndExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedBitAndExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitXor, std::move(result_ptr),
      buildBitAndExpr(ctx->closedBitAndExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtBitAndExpr(
    rx::RxParser::StatementBitAndExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr =
      buildStmtShiftExpr(ctx->statementShiftExpression());
  for (int i = 0; i < ctx->shiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->shiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildShiftExpr(ctx->shiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtBitAndExpr(
    rx::RxParser::StatementClosedBitAndExpressionContext *ctx)
{
  if (ctx->AMP().size() == 0)
  {
    return buildStmtShiftExpr(ctx->statementClosedShiftExpression());
  }
  std::unique_ptr<Expr> result_ptr =
      buildStmtShiftExpr(ctx->statementShiftExpression());
  for (int i = 0; i < ctx->shiftExpression().size(); i++)
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->shiftExpression()[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_ptr = std::make_unique<BinaryExpr>(
        BinaryOperator::BitAnd, std::move(result_ptr),
        buildShiftExpr(ctx->shiftExpression()[i]));
    result_ptr->span_ = node_span;
  }
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedShiftExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  result_ptr = std::make_unique<BinaryExpr>(
      BinaryOperator::BitAnd, std::move(result_ptr),
      buildShiftExpr(ctx->closedShiftExpression()));
  result_ptr->span_ = node_span;
  return result_ptr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtShiftExpr(
    rx::RxParser::StatementShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::StatementAdditiveExpressionContext *>(
            child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::StatementAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildStmtAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::StatementClosedAdditiveExpressionContext
                              *>(child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::StatementClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildStmtAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
    else if (dynamic_cast<rx::RxParser::AdditiveExpressionContext *>(
                 child_context))
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}
std::unique_ptr<Expr> AstBuilder::buildStmtShiftExpr(
    rx::RxParser::StatementClosedShiftExpressionContext *ctx)
{
  std::unique_ptr<Expr> result_ptr;
  BinaryOperator now_op;
  for (auto *child_context : ctx->children)
  {
    if (dynamic_cast<rx::RxParser::StatementAdditiveExpressionContext *>(
            child_context))
    {
      auto *add_expr_ctx =
          dynamic_cast<rx::RxParser::StatementAdditiveExpressionContext *>(
              child_context);
      std::unique_ptr<Expr> add_expr = buildStmtAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftRight;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftRight;
      }
    }
    else if (dynamic_cast<rx::RxParser::StatementClosedAdditiveExpressionContext
                              *>(child_context))
    {
      auto *add_expr_ctx = dynamic_cast<
          rx::RxParser::StatementClosedAdditiveExpressionContext *>(
          child_context);
      std::unique_ptr<Expr> add_expr = buildStmtAddExpr(add_expr_ctx);
      if (result_ptr == nullptr)
      {
        result_ptr = std::move(add_expr);
        now_op = BinaryOperator::ShiftLeft;
      }
      else
      {
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
    else if (dynamic_cast<rx::RxParser::AdditiveExpressionContext *>(
                 child_context))
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(add_expr_ctx->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_ptr = std::make_unique<BinaryExpr>(now_op, std::move(result_ptr),
                                                  std::move(add_expr));
        result_ptr->span_ = node_span;
        now_op = BinaryOperator::ShiftLeft;
      }
    }
  }
  return std::move(result_ptr);
}
std::unique_ptr<Expr> AstBuilder::buildStmtAddExpr(
    rx::RxParser::StatementAdditiveExpressionContext *ctx)
{
  auto mult_expr_context = ctx->statementMultiplicativeExpression();
  auto mult_expr_contexts = ctx->multiplicativeExpression();
  auto operators = ctx->additiveOperator();

  std::unique_ptr<Expr> result_expr = buildStmtMultExpr(mult_expr_context);
  for (int i = 0; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  return result_expr;
}
std::unique_ptr<Expr> AstBuilder::buildStmtAddExpr(
    rx::RxParser::StatementClosedAdditiveExpressionContext *ctx)
{
  auto mult_expr_contexts = ctx->multiplicativeExpression();
  auto operators = ctx->additiveOperator();
  if (!operators.size())
  {
    return buildStmtMultExpr(ctx->statementClosedMultiplicativeExpression());
  }
  std::unique_ptr<Expr> result_expr =
      buildStmtMultExpr(ctx->statementMultiplicativeExpression());
  for (int i = 0; i < mult_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getAddOperator(operators[i]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(mult_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildMultExpr(mult_expr_contexts[i]));
    result_expr->span_ = node_span;
  }

  BinaryOperator last_op = getAddOperator(operators.back());
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last =
      spanOf(ctx->closedMultiplicativeExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result = std::make_unique<BinaryExpr>(
      last_op, std::move(result_expr),
      buildMultExpr(ctx->closedMultiplicativeExpression()));
  result->span_ = node_span;
  return result;
}
std::unique_ptr<Expr> AstBuilder::buildStmtMultExpr(
    rx::RxParser::StatementMultiplicativeExpressionContext *ctx)
{
  if (ctx->multiplicativeOperator().size())
  {
    std::unique_ptr<Expr> result_expr =
        buildStmtCastExpr(ctx->statementCastExpression());
    for (int i = 0; i < ctx->castExpression().size(); i++)
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(ctx->castExpression()[i]->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_expr = std::make_unique<BinaryExpr>(
          getMultOperator(ctx->multiplicativeOperator()[i]),
          std::move(result_expr), buildCastExpr(ctx->castExpression()[i]));
      result_expr->span_ = node_span;
    }
    return std::move(result_expr);
  }
  return buildStmtCastExpr(ctx->statementCastExpression());
}
std::unique_ptr<Expr> AstBuilder::buildStmtMultExpr(
    rx::RxParser::StatementClosedMultiplicativeExpressionContext *ctx)
{
  auto cast_expr_contexts = ctx->castExpression();
  auto operators = ctx->multiplicativeOperator();
  if (!operators.size())
  {
    return buildStmtCastExpr(ctx->statementClosedCastExpression());
  }
  std::unique_ptr<Expr> result_expr =
      buildStmtCastExpr(ctx->statementCastExpression());
  for (int i = 0; i < cast_expr_contexts.size(); ++i)
  {
    BinaryOperator op = getMultOperator(operators[i]);
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(cast_expr_contexts[i]->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr = std::make_unique<BinaryExpr>(
        op, std::move(result_expr), buildCastExpr(cast_expr_contexts[i]));
    result_expr->span_ = node_span;
  }
  BinaryOperator last_op = getMultOperator(operators[operators.size() - 1]);
  const auto node_span_first = spanOf(ctx->getStart());
  const auto node_span_last = spanOf(ctx->closedCastExpression()->getStop());
  std::optional<SourceSpan> node_span;
  if (node_span_first && node_span_last &&
      node_span_last->end >= node_span_first->begin)
  {
    node_span = SourceSpan{node_span_first->begin, node_span_last->end};
  }
  auto result =
      std::make_unique<BinaryExpr>(last_op, std::move(result_expr),
                                   buildCastExpr(ctx->closedCastExpression()));
  result->span_ = node_span;
  return result;
}
std::unique_ptr<Expr>
AstBuilder::buildStmtCastExpr(rx::RxParser::StatementCastExpressionContext *ctx)
{
  if (ctx->AS().size())
  {
    std::unique_ptr<Expr> result_ptr =
        buildStmtUnaryExpr(ctx->statementUnaryExpression());
    for (auto type_ctx : ctx->typeRef())
    {
      const auto node_span_first = spanOf(ctx->getStart());
      const auto node_span_last = spanOf(type_ctx->getStop());
      std::optional<SourceSpan> node_span;
      if (node_span_first && node_span_last &&
          node_span_last->end >= node_span_first->begin)
      {
        node_span = SourceSpan{node_span_first->begin, node_span_last->end};
      }
      result_ptr = std::make_unique<CastExpr>(std::move(result_ptr),
                                              buildType(type_ctx));
      result_ptr->span_ = node_span;
    }
    return std::move(result_ptr);
  }
  return buildStmtUnaryExpr(ctx->statementUnaryExpression());
}
std::unique_ptr<Expr> AstBuilder::buildStmtCastExpr(
    rx::RxParser::StatementClosedCastExpressionContext *ctx)
{
  if (ctx->AS())
  {
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->closedCastType()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    auto result = std::make_unique<CastExpr>(
        buildStmtCastExpr(ctx->statementCastExpression()),
        buildClosedCastType(ctx->closedCastType()));
    result->span_ = node_span;
    return result;
  }
  return buildStmtUnaryExpr(ctx->statementUnaryExpression());
}
std::unique_ptr<Expr> AstBuilder::buildStmtUnaryExpr(
    rx::RxParser::StatementUnaryExpressionContext *ctx)
{
  if (ctx->unaryOperator())
  {
    auto op = ctx->unaryOperator();
    auto expr = buildUnaryExpr(ctx->unaryExpression());
    if (op->MINUS())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Negate, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->NOT())
    {
      auto result =
          std::make_unique<UnaryExpr>(UnaryOperator::Not, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->STAR())
    {
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Dereference,
                                                std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else if (op->AMP())
    {
      UnaryOperator kind =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto result = std::make_unique<UnaryExpr>(kind, std::move(expr));
      result->span_ = spanOf(ctx);
      return result;
    }
    else
    {
      UnaryOperator kind_in =
          op->MUT() ? UnaryOperator::BorrowMut : UnaryOperator::Borrow;
      auto inner_span = spanOf(ctx);
      if (inner_span && inner_span->begin < inner_span->end)
      {
        ++inner_span->begin;
      }
      else
      {
        inner_span = std::nullopt;
      }
      auto inner_node = std::make_unique<UnaryExpr>(kind_in, std::move(expr));
      inner_node->span_ = inner_span;
      auto result = std::make_unique<UnaryExpr>(UnaryOperator::Borrow,
                                                std::move(inner_node));
      result->span_ = spanOf(ctx);
      return result;
    }
  }
  else
  {
    return buildStmtPostfixExpr(ctx->statementPostfixExpression());
  }
}
std::unique_ptr<Expr> AstBuilder::buildStmtPostfixExpr(
    rx::RxParser::StatementPostfixExpressionContext *ctx)
{
  if (ctx->nonBlockPrimary())
  {
    std::unique_ptr<Expr> result_expr =
        buildNonBlockPrimary(ctx->nonBlockPrimary());
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(postfixs[i]->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_expr = std::make_unique<CallExpr>(std::move(result_expr),
                                                 std::move(arguments));
        result_expr->span_ = node_span;
      }
      else if (postfixs[i]->expression())
      {
        std::unique_ptr<Expr> index_expr = buildExpr(postfixs[i]->expression());
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(postfixs[i]->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_expr = std::make_unique<IndexExpr>(std::move(result_expr),
                                                  std::move(index_expr));
        result_expr->span_ = node_span;
      }
      else
      {
        result_expr =
            buildDotSuffix(std::move(result_expr), postfixs[i]->dotSuffix());
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(postfixs[i]->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_expr->span_ = node_span;
      }
    }
    return result_expr;
  }
  else
  {
    std::unique_ptr<Expr> result_expr =
        buildExprWithBlock(ctx->expressionWithBlock());
    result_expr = buildDotSuffix(std::move(result_expr), ctx->dotSuffix());
    const auto node_span_first = spanOf(ctx->getStart());
    const auto node_span_last = spanOf(ctx->dotSuffix()->getStop());
    std::optional<SourceSpan> node_span;
    if (node_span_first && node_span_last &&
        node_span_last->end >= node_span_first->begin)
    {
      node_span = SourceSpan{node_span_first->begin, node_span_last->end};
    }
    result_expr->span_ = node_span;
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
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(postfixs[i]->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_expr = std::make_unique<CallExpr>(std::move(result_expr),
                                                 std::move(arguments));
        result_expr->span_ = node_span;
      }
      else if (postfixs[i]->expression())
      {
        std::unique_ptr<Expr> index_expr = buildExpr(postfixs[i]->expression());
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(postfixs[i]->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_expr = std::make_unique<IndexExpr>(std::move(result_expr),
                                                  std::move(index_expr));
        result_expr->span_ = node_span;
      }
      else
      {
        result_expr =
            buildDotSuffix(std::move(result_expr), postfixs[i]->dotSuffix());
        const auto node_span_first = spanOf(ctx->getStart());
        const auto node_span_last = spanOf(postfixs[i]->getStop());
        std::optional<SourceSpan> node_span;
        if (node_span_first && node_span_last &&
            node_span_last->end >= node_span_first->begin)
        {
          node_span = SourceSpan{node_span_first->begin, node_span_last->end};
        }
        result_expr->span_ = node_span;
      }
    }
    return result_expr;
  }
}
std::unique_ptr<Expr>
AstBuilder::buildExprWithBlock(rx::RxParser::ExpressionWithBlockContext *ctx)
{
  if (ctx->ifExpression())
  {
    return buildIfExpr(ctx->ifExpression());
  }
  else if (ctx->LOOP())
  {
    auto result =
        std::make_unique<LoopExpr>(buildBlockExpr(ctx->blockExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  else if (ctx->WHILE())
  {
    auto result = std::make_unique<WhileExpr>(
        buildConditionExpr(ctx->conditionExpression()),
        buildBlockExpr(ctx->blockExpression()));
    result->span_ = spanOf(ctx);
    return result;
  }
  else
  {
    return buildBlockExpr(ctx->blockExpression());
  }
}
