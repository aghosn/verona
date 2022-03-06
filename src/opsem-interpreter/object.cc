#include "error.h"
#include "object.h"

#include <sstream>

unsigned int objectid = 0;

namespace interpreter
{
  std::string nextObjectId()
  {
    std::stringstream ss;
    std::string s;
    ss << objectid++;
    ss >> s;
    return s;
  }

  // Converts a strategy to a region type if the strategy is NOT unsafe.
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

  Region::Region(ir::AllocStrategy s)
  {
    strategy = s; 
    auto strat = strategyToRegionType(strategy); 
    rt_region = new (strat) VObject;
    sb = nullptr;
  }

  Region::Region(std::shared_ptr<interop::SandboxConfig> s)
  {
    strategy = ir::AllocStrategy::Unsafe;
    rt_region = nullptr;
    sb = s;
  }

  VObject* Region::alloc(void)
  {
    if (strategy < ir::AllocStrategy::Unsafe)
    {
      CHECK(rt_region != nullptr, E_NULL); 
      CHECK(sb == nullptr, E_NON_NULL);

      // The region is the current opened one.
      return new VObject(); 
    }
    CHECK(strategy == ir::AllocStrategy::Unsafe, 
        E_WRONG_KIND(ir::AllocStrategy::Unsafe, strategy));
    CHECK(rt_region == nullptr, E_NON_NULL);
    CHECK(sb != nullptr, E_NULL);
    
    return (VObject*)(sb->lib->alloc_in_sandbox(sizeof(VObject), 1)); 
  }

} // namespace interpreter
