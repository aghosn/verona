#pragma once

#include <vector>
#include <clang/AST/Decl.h>

#include "CXXQuery.h"

using namespace std;

namespace verona::interop {

  // TODO: remove afterwards, for the moment we do it like this.
  static const string targets[] = {
    "func1",
    "func2",
  };

  void specialize_export_function(const CXXQuery* query);

} // namespace verona::interop;
