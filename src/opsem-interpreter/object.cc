#include "error.h"
#include "object.h"

#include <sstream>

unsigned int objectid = 0;

namespace interpreter
{

  /*
   * Helper functions to generate ObjectId and convert strategies between
   * ir and rt representations.
   * FIXME eventually use object address as unique Id and merge the notion of 
   * an unsafe region into rt.
   */
  std::string nextObjectId()
  {
    std::stringstream ss;
    std::string s;
    ss << objectid++;
    ss >> s;
    return s;
  }

  rt::RegionType strategyToRegionType(ir::AllocStrategy strat)
  {
    CHECK(strat < ir::AllocStrategy::Unsafe, E_STRAT_TO_REGION_TYPE);
    switch(strat)
    {
      case ir::AllocStrategy::Trace:
        return rt::RegionType::Trace;
      case ir::AllocStrategy::Arena:
        return rt::RegionType::Arena;
      case ir::AllocStrategy::Rc:
        return rt::RegionType::Rc;
      default:
        CHECK(0, E_MISSING_CASE); 
    }
    // Should never get here.
    return rt::RegionType::Trace;
  }

  /*
   * Object methods.
   */

  Shared<ir::StorageLoc> Object::getStorageLoc(Id name)
  {
    return fields[name]; 
  }

  /*
   * Region methods
   */
  
  Region::Region(ir::AllocStrategy s)
  {
    CHECK(s < ir::AllocStrategy::Unsafe, E_UNSAFE_NEW_REGION);
    strategy = s;
    auto strat = strategyToRegionType(strategy);
    inner.rt_region = new (strat) Object;
  }

  Region::Region(interop::SandboxConfig* sb)
  {
    CHECK(sb != nullptr, E_NULL);
    strategy = ir::AllocStrategy::Unsafe;
    inner.sandbox = sb;
  }

  Object* Region::alloc(void)
  {
    if (strategy < ir::AllocStrategy::Unsafe)
    {
      CHECK(inner.rt_region != nullptr, E_NULL);
      return new Object();
    }
    CHECK(strategy == ir::AllocStrategy::Unsafe,
        E_WRONG_KIND(ir::AllocStrategy::Unsafe, strategy));
    CHECK(inner.sandbox != nullptr, E_NULL);
    return (Object*)(inner.sandbox->lib->alloc_in_sandbox(sizeof(Object), 1));
  }

  void Region::open(void)
  {
    if (strategy == ir::AllocStrategy::Unsafe)
      return;
    rt::api::open_region(inner.rt_region);
  }

  interop::SandboxConfig* Region::sandbox()
  {
    if (strategy != ir::AllocStrategy::Unsafe)
      return nullptr;
    return inner.sandbox;
  }
} // namespace interpreter
