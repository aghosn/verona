#pragma once

#include <atomic>
#include <cstdint>
#include <cassert>

/// NO_PREEMPT macro creates a local Preempt variable that increments
/// the thread-local counter. As the local variable's scope ends, the counter
/// is decremented. This simple mechanism thus supports nested runtime calls 
/// preemption disabling.
#if defined(USE_SYSMONITOR) and defined(USE_PREEMPTION)
#define NO_PREEMPT() verona::rt::Preempt __nopreempt;  
#else
#define NO_PREEMPT() {}
#endif

typedef void(*fncast)(void*);


#if defined(USE_SYSMONITOR) and defined(USE_PREEMPTION)

#define DISABLE_MASK ((uint64_t)(1) << 63)

namespace verona::rt
{

  /// The Preempt class holds useful logic to handle preemption:
  /// 1)  It defines a thread-local counter that is incremented upon entering 
  ///     a runtime function and decremented as we leave it. If a thread's 
  ///     counter is != 0, the thread cannot be preempted.
  class Preempt
  {

    public:
      inline static std::atomic<uint64_t> preempted_address;
      inline static std::atomic<uint64_t> preempted_count;
      inline static std::atomic<uint64_t> preempted_interrupts;
      /// Instantiation of a preempt object increments the counter.
      Preempt()
      {
        counter()++;
      }

      /// Deallocation of a preempt object decrements the counter.
      ~Preempt()
      {
        std::atomic<uint64_t>& c = counter();
        if (c == 0)
          abort();
        c--;
      }

      /// Whether the current thread can be preempted.
      static bool is_preemptable()
      {
        std::atomic<uint64_t>& c = counter();
        return (c == 0 /*&& flag()*/);
      }

      /// Turn off preemption regardless of counter.
      static void disable_preemption()
      {
        std::atomic<uint64_t>& c = counter();
        c |= DISABLE_MASK;
      }

      /// Re-enables preemption.
      /// This does not reset the counter.
      static void reenable_preemption()
      {
        std::atomic<uint64_t>& c = counter();
        c &= ~(DISABLE_MASK);
      }



      static bool check_counter(uint64_t value)
      {
        std::atomic<uint64_t>& c = counter();
        uint64_t v = c & ~(DISABLE_MASK);
        return v == value;
      }

    private:
      /// The _counter is defined as thread local here within a static function.
      /// This is the only viable way we found to define this variable while
      /// protecting it from potential modifications.
      /// Note that this mechanism relies on TLS and might not work with sandboxes.
      static std::atomic<uint64_t>& counter()
      {
        // Disable preemption at thread creation.
        static thread_local std::atomic<uint64_t> _counter = DISABLE_MASK;
        return _counter;
      }
  };
} //namespace verona::rt;

#endif
