#pragma once
#include "ir.h"
#include "type.h"
#include "utils.h"
#include "interoperability.h"

#include <cassert>
#include <verona.h>
#include <memory>

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

  //TODO figure that out
  struct Region {
    ir::AllocStrategy strategy;

    //TODO we need to surface the snmalloc region somehow.
    //For the moment we either have a pointer to the SandboxConfig,
    //or a verona runtime region, i.e., an object.
    VObject* rt_region;
    std::shared_ptr<interop::SandboxConfig> sb; 

    Region(ir::AllocStrategy);
    Region(std::shared_ptr<interop::SandboxConfig> s);

    // Allocator for memory in the region
    // FIXME Later add a size?
    VObject* alloc(void);
  };

  
  // A Region is just a verona object.
  //using Region = VObject;

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
  rt::RegionType strategyToRegionType(ir::AllocStrategy strat);
} // namespace interpreter
