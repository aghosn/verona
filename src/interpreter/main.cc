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
  if (parser.parse()) {
    cout << "Done parsing" << endl;
  } else {
    cout << "Could not parse" << endl;
  } 
  return 0;
}
