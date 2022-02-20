#pragma once

#include <string>
#include <vector>

using namespace std;

namespace verona::interop::api
{

  /// Parse config file adding args to the args globals
  void parseCommandLine(int argc, char** argv, vector<string>& includePath);

  /// Runs the interop.
  void run(int argc, char** argv);

} // namespace verona::interop::api
