#include <iostream>
#include <vector>
#include <string>

#include "lexer.h"

#include "helpers.h"

using namespace std;

int main(int argc, const char** argv) {
  mlexer::Lexer lexer("tests/test.txt");

 //for(auto line: lexer.lines) {
 //   for (auto tok: line.tokens) {
 //     cout << "'"<< tok.kind << "'";
 //   }
 //   cout << endl;
 //}  
 
  return 0;
}
