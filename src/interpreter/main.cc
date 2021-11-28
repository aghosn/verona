#include "interpreter.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"

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

  Interpreter interp; // = Interpreter();

  for (auto l : parser.program)
  {
    auto res = interp.match(l);
    if (res.size() == 0)
    {
      cout << "No match" << endl;
    }
    for (auto r : res)
    {
      cout << kindname(l->kind()) << "matches " << r->print() << endl;
    }
  }
  return 0;
}
