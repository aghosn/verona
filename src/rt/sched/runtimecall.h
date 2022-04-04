// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT
#pragma once

namespace verona::rt {
  // TODO can we merge that into the class?
  thread_local static bool in_runtime_call = false;

#ifdef ENABLE_PREEMPTION
#define MARK_RT_FUNCTION RuntimeCall __verona_rt_marker;
#else
#define MARK_RT_FUNCTION ;;
#endif

  class RuntimeCall
  {
    bool nested;
    public:
      RuntimeCall()
      {
        nested = false;
        if (in_runtime_call) {
         nested = true; 
        }
        in_runtime_call = true;
      }

      ~RuntimeCall()
      {
        if (!nested) {
          in_runtime_call = false;
        }
      }

      static bool isInRuntimeCall() {
        return in_runtime_call;
      }    
  };
}
