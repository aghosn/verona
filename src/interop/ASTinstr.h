#pragma once

#include "CXXInterface.h"

#include <clang/AST/Decl.h>
#include <vector>

using namespace std;

namespace verona::interop
{
  // Contains all the library functions exposed via the sandbox API.
  // Set up by the command line argument parser.
  extern vector<string> target_functions;

  // The fully qualified name for the exporter class.
  // It must have an 'export_function' static method.
  extern string exporter_class_name; 

  void specialize_export_function(CXXInterface& interface);

} // namespace verona::interop;
