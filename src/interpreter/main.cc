#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "visitors.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace verona::ir;
using namespace interpreter;

int main(int argc, const char** argv)
{
  mlexer::Lexer lexer("tests/test.txt");

  while (lexer.hasNext())
  {
    cout << " " << mlexer::tokenkindname(lexer.next().kind);
  }
  lexer.reset();

  Parser parser(lexer);
  parser.parse();

  SimplePrinter printer;

  for (auto l : parser.program)
  {
    l->accept(&printer);
  }

  return 0;
}
