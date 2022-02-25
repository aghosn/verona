#include "interoperability.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <Interop.h>
#include <filesystem> // C++17

using namespace std;
namespace interpreter::interop
{

  std::unique_ptr<SandboxConfig> initializeLibrary(std::string name, std::string config)
  {
    // Nothing to do
    if (config.empty())
    {
      return nullptr;
    }

    std::ifstream file(config);
    if (!file.good())
    {
      std::cerr << "Error opening interoperability config file " << config << std::endl; 
      exit(1);
    }

    // We read the file line by line, there should be one argument per line.
    vector<std::string> arguments;
    std::string line;
    while(std::getline(file, line))
    {
      arguments.push_back(line);
    }

    //FIXME The API takes a char** and not a vector, that's ugly.
    char** args = (char**) calloc(sizeof(char*), arguments.size());
    for (int i = 0; i < arguments.size(); i++)
    {
      args[i] = (char*)calloc(sizeof(char), arguments[i].size()+1);
      strcpy(args[i], arguments[i].c_str());
    }
    auto library = verona::interop::api::run(arguments.size(), args);
    std::cout << "The dynamic library is in " << library << std::endl;

    // Free everything, hopefully the arguments have been copied...
    for (int i = 0; i < arguments.size(); i++)
    {
      free(args[i]);
    }
    free(args);

    // create the return value;
    auto sbconfig = std::make_unique<SandboxConfig>();
    sbconfig->config = config;
    sbconfig->name = name;
    return sbconfig;
  }
} // namespace interpreter::interop
