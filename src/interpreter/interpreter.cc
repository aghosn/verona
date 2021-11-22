#include <iostream>

#include "lexer.h"
#include "ir.h"

using namespace std;
using namespace verona::parser;

int main(int argc, char** argv) {
  cout << "Hello world!" << endl;
  auto file = string("/home/aghosn/Documents/Programs/MSR/Forks/verona/src/interpreter/tests/lexer.txt");
  Source src = load_source(file);
  size_t pos = 0;
  Token tok = lex(src, pos);
  return 0;
}
