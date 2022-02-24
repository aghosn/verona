#pragma once

#include <string>
#include <vector>

namespace verona::interop::api
{

  /// Parse config file adding args to the args globals
  void parseCommandLine(int argc, char** argv, std::vector<std::string>& includePath);

  /// Runs the interop.
  std::string run(int argc, char** argv);

} // namespace verona::interop::api
