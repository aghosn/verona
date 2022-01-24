#pragma once
#include "ir.h"
#include "type.h"
#include "utils.h"

#include <cassert>
#include <verona.h>

using ObjectId = std::string;

namespace interpreter
{
  /**
   * TODO Figure out if we do want to wrap the complete structures
   * in verona allocations, or if we just leave them in our memory
   * and rely on verona just for the data-holding memory.
   */

  // An object
  struct VObject : rt::V<VObject> 
  {
    void trace(rt::ObjectStack& st) const
    {
    }
  };

  
  // A Region is just a verona object.
  using Region = VObject;

  // ω ∈ Object ::= Region* × TypeId
  struct Object
  {
    List<Region*> regions;
    TypeId type;

    // TODO figure out whether this is correct
    Map<Id, Shared<ir::StorageLoc>> fields;

    VObject* obj;

    // For convenience, we store the ObjectId here as well.
    ObjectId id;

    Shared<ir::StorageLoc> getStorageLoc(Id name)
    {
      return fields[name];
    }
  };

  std::string nextObjectId();
} // namespace interpreter
