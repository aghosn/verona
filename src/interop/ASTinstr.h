#pragma once

#include <vector>
#include <clang/AST/Decl.h>

#include "CXXInterface.h"

using namespace std;

namespace verona::interop {
  // Contains all the library functions exposed via the sandbox API.
  // Set up by the command line argument parser.
  static vector<string> target_functions;

  void specialize_export_function(CXXInterface& interface);

} // namespace verona::interop;
