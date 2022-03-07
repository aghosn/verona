#include "interoperability.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <Interop.h>
#include <filesystem> // C++17
#include <cassert>

#include <process_sandbox/cxxsandbox.h>
#include <process_sandbox/sandbox.h>

using namespace std;
namespace interpreter::interop
{
  std::shared_ptr<SandboxConfig> initializeLibrary(std::string config)
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
    // Fake the program name as first argument
    arguments.push_back("begin");
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
    std::cout << "[EVAL] Sandbox: " << library << std::endl;

    // Free everything, hopefully the arguments have been copied...
    for (int i = 0; i < arguments.size(); i++)
    {
      free(args[i]);
    }
    free(args);

    // create the return value;
    auto sbconfig = std::make_shared<SandboxConfig>();
    sbconfig->config = config;
    sbconfig->lib = std::make_unique<sandbox::Library>(library.c_str());
    int i = 0; 
    for (auto target: verona::interop::target_functions)
    {
      sbconfig->targets[target] = i;
      i++;
    }
    return sbconfig;
  }

//  // TODO remove afterwards, I'm just testing that things work;
//  void test_sandbox() {
//    lib = std::make_unique<sandbox::Library>("/tmp/sandboxed-basic.so");
//  }
//  void invoke_target()
//  {
//    assert(lib != nullptr); 
//    struct {
//      int a;
//      int b;
//      int ret;
//    } args;
//    args.a = 1;
//    args.b = 2;
//    args.ret = 0;
//    lib->send(0, (void*) &args);
//  }
} // namespace interpreter::interop
