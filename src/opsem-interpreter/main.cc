#include "interpreter.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "visitors.h"

#include <iostream>
#include <string>
#include <vector>
#include <verona.h>

using namespace std;
using namespace verona::ir;
using namespace interpreter;
using namespace verona::rt::api;

int main(int argc, const char** argv)
{
  if (argc < 2)
  {
    cerr << "No file supplied." << endl;
    exit(1);
  }
  mlexer::Lexer lexer(argv[1]);

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

  Interpreter interp(parser);
  return 0;
}
