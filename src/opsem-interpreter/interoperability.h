#pragma once

#include <memory>
#include <string>

namespace interpreter::interop 
{
  // TODO should we also have the targets here?
  struct SandboxConfig
  {
    std::string config;
    std::string name;
  };

  /**
   * Take a path to a `config` file that should contain the configuration
   * for the C/C++ library we need to compile, link, and import to run inside
   * a sandbox.
   */
  std::unique_ptr<SandboxConfig> initializeLibrary(std::string name, std::string config);

} // namespace interpreter::interop
