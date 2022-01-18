#pragma once

#include "utils.h"

using TypeId = std::string;

namespace interpreter {
  // Type ::= TypeId* × (Id → Member)
  struct Type {
    TypeId name;
    Map<Id, Shared<ir::Member>> members;
  };
} //namespace interpreter
