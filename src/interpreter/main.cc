#include <iostream>
#include <vector>
#include <string>

#include "lexer.h"
#include "parser.h"

#include "ir.h"

using namespace std;
using namespace verona::ir;

int main(int argc, const char** argv) {
  mlexer::Lexer lexer("tests/test.txt");
  
  while (lexer.hasNext()) {
    cout << " " << mlexer::tokenkindname(lexer.next().kind);
  }
  lexer.reset();
  
  Parser parser(lexer);
  parser.parse();
  
  for (auto l: parser.program) {
    cout << kindname(l->kind()) << endl; 
  }
  return 0;
}
