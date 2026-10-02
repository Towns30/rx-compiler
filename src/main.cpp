#include <antlr4-runtime.h>

#include <fstream>
#include <iostream>

#include "RxLexer.h"
#include "RxParser.h"

void printTree(antlr4::tree::ParseTree *node, const rx::RxParser &parser,
               int depth = 0)
{
  std::cout << std::string(depth * 2, ' ');
  if (auto *terminal = dynamic_cast<antlr4::tree::TerminalNode *>(node))
  {
    std::cout << "'" << terminal->getText() << "'\n";
    return;
  }
  if (auto *rule = dynamic_cast<antlr4::ParserRuleContext *>(node))
  {
    std::size_t index = rule->getRuleIndex();
    std::cout << parser.getRuleNames().at(index) << '\n';
  }
  else
  {
    std::cout << node->getText() << '\n';
  }

  for (auto *child : node->children)
  {
    printTree(child, parser, depth + 1);
  }
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: rx-parser <source.rx>\n";
    return 2;
  }
  std::ifstream source(argv[1]);
  if (!source)
  {
    std::cerr << "Cannot open file: " << argv[1] << '\n';
    return 2;
  }
  antlr4::ANTLRInputStream input(source);
  rx::RxLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  rx::RxParser parser(&tokens);
  auto *tree = parser.crate();
  printTree(tree, parser);
}