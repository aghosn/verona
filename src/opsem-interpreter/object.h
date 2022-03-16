#pragma once

#include "type.h"
#include "utils.h"
#include "interoperability.h"

#include <string>
#include <memory>
#include <verona.h>

using ObjectId = std::string;

namespace interpreter
{
  struct Region;

  struct Object : public rt::V<Object>
  {
    ObjectId id;
    TypeId type;

    List<Region*> regions;

    Map<ObjectId, Shared<ir::StorageLoc>> fields;

    Shared<ir::StorageLoc> getStorageLoc(Id name);

    void trace(rt::ObjectStack& st) {}
  };

  struct Region
  {
    ir::AllocStrategy strategy;
    
    union Inner
    {
      interop::SandboxConfig* sandbox;
      Object* rt_region;
    };
    
    Region(ir::AllocStrategy);
    Region(interop::SandboxConfig* sandbox);

    // TODO: Make this a template?
    Object* alloc(void);
    void open(void);
    interop::SandboxConfig* sandbox();

    private:
    Inner inner;
  };

  std::string nextObjectId();
  rt::RegionType strategyToRegionType(ir::AllocStrategy strat);
} // namespace interpreter
