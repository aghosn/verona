#pragma once

#include "type.h"
#include "utils.h"

using ObjectId = std::string;

namespace interpreter {
  
  // ω ∈ Object ::= Region* × TypeId
  struct Object {
    List<rt::Region*> regions;
    TypeId type;

    //TODO figure out if we need this.
    rt::Object* obj;
  };

  // f ∈ StorageLoc ::= ObjectId × Id
  struct StorageLoc {
    ObjectId object;
    Id id;
  };

} // namespace interpreter
