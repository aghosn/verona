#pragma once
#include <cassert>

#include "ir.h"
#include "type.h"
#include "utils.h"

using ObjectId = std::string;

namespace interpreter {
  // ω ∈ Object ::= Region* × TypeId
  struct Object {
    List<rt::Region*> regions;
    TypeId type;

    //TODO figure out whether this is correct
    Map<Id, Shared<ir::StorageLoc>> fields;

    //TODO figure out if we need this.
    rt::Object* obj;

    Shared<ir::StorageLoc> getStorageLoc(Id name) {
      return fields[name];
    }
  }; 
} // namespace interpreter
