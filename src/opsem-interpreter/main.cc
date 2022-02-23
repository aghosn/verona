#include "interpreter.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "visitors.h"
#include "interoperability.h"

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
  //FIXME I just want some code to run, come back and fix the handling of args.
  //TODO(aghosn)
  if (argc < 2)
  {
    cerr << "No file supplied." << endl;
    exit(1);
  }
  if (argc > 3)
  {
    cerr << "Too many arguments supplied" << endl;
    exit(1);
  }
  std::string libsandbox = "";
  std::string inputFile = argv[1];
  if (argc == 3)
  {
     libsandbox = argv[1];
     inputFile = argv[2];
  }
    
  mlexer::Lexer lexer(inputFile);

  Parser parser(lexer);
  parser.parse();
  
  if (!libsandbox.empty())
  {
    //FIXME do something cleaner, check how to pass that to the interpreter. 
    interop::initializeLibrary("mylib", libsandbox);
  }
  Interpreter interp(&parser);
  interp.eval();
  return 0;
}
