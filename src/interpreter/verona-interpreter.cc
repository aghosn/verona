#include "parser/parser.h"
#include "parser/path.h"


#include <iostream>
#include <CLI/CLI.hpp>

using namespace verona::parser;

constexpr auto stdlib = "stdlib/";

int main(int argc, char** argv) {
  CLI::App app{"Verona Interpreter"};
  std::string path;

  app.add_option("path", path, "Path to the module to interpret")->required();

  try 
  {
    app.parse(argc, argv);
  }
  catch(const CLI::ParseError& e)
  {
    return app.exit(e);
  }

  auto stdlibpath = path::canonical(path::join(path::executable(), stdlib));
  auto [ok, ast] = parse(path, stdlibpath);
  if (!ok)
  {
    std::cerr << "Error parsing file: " << path << std::endl;
     return -1;
  }
  std::cout << "We survived " << kindname(ast->kind()) << std::endl;
  return 0;
}
