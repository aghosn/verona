#pragma once
#include "ir.h"
#include "type.h"
#include "utils.h"

#include <cassert>
#include <verona.h>

using ObjectId = std::string;

namespace interpreter
{
  // TODO fix once I know how to represent regions.
  typedef rt::Object Region;

  // ω ∈ Object ::= Region* × TypeId
  struct Object
  {
    List<Region*> regions;
    TypeId type;

    // TODO figure out whether this is correct
    Map<Id, Shared<ir::StorageLoc>> fields;

    // TODO figure out if we need this.
    rt::Object* obj;

    // For convenience, we store the ObjectId here as well.
    ObjectId id;

    Shared<ir::StorageLoc> getStorageLoc(Id name)
    {
      return fields[name];
    }
  };

  // TODO Probably not correct
  using VObject = rt::V<Object>;

  std::string nextObjectId();
} // namespace interpreter
