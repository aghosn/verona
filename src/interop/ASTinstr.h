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

  // The name of the exporter_class_name's method responsible for exports.
  // It must be a template class that depends on the ret and arg types.
  extern const char* METHOD_NAME;

  void generate_dispatchers(CXXInterface& interface);

  void specialize_export_function(CXXInterface& interface);

} // namespace verona::interop;
