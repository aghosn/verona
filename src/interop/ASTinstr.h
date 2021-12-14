#pragma once

#include <vector>
#include <clang/AST/Decl.h>

#include "CXXInterface.h"

using namespace std;

namespace verona::interop {

  // TODO: remove afterwards, for the moment we do it like this.
  static const string targets[] = {
    "func1",
    "func2",
  };

  void specialize_export_function(CXXInterface& interface);

} // namespace verona::interop;
