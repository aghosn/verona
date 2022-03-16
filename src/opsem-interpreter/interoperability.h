#pragma once

#include <map>
#include <memory>
#include <string>
#include <process_sandbox/cxxsandbox.h>

namespace interpreter::interop 
{
  struct SandboxConfig
  {
    std::string dynpath;
    std::string config;
    std::unique_ptr<sandbox::Library> lib;
    //TODO this will need to have more information than just this.
    //We will need the function prototype.
    std::map<std::string, int> targets;
  };

  /**
   * Take a path to a `config` file that should contain the configuration
   * for the C/C++ library we need to compile, link, and import to run inside
   * a sandbox.
   */
  SandboxConfig* initializeLibrary(std::string config);
  void test_sandbox();
  void invoke_target();
} // namespace interpreter::interop
