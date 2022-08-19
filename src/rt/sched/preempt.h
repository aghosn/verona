#pragma once

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

namespace verona::rt
{

  /// The Preempt class holds useful logic to handle preemption:
  /// 1)  It defines a thread-local counter that is incremented upon entering 
  ///     a runtime function and decremented as we leave it. If a thread's 
  ///     counter is != 0, the thread cannot be preempted.
  class Preempt
  {

    public:
      /// Instantiation of a preempt object increments the counter.
      Preempt()
      {
        counter()++;
      }

      /// Deallocation of a preempt object decrements the counter.
      ~Preempt()
      {
        size_t& c = counter();
        if (c == 0)
          abort();
        c--;
      }

      /// Whether the current thread can be preempted.
      static bool is_preemptable()
      {
        return (counter() == 0 && flag());
      }

      /// Turn off preemption regardless of counter.
      static void disable_preemption()
      {
        bool& _flag = flag();
        _flag = false;
      }

      /// Re-enables preemption.
      /// This does not reset the counter.
      static void reenable_preemption()
      {
        bool& _flag = flag();
        _flag = true;
      }

    private:

      /// The _counter is defined as thread local here within a static function.
      /// This is the only viable way we found to define this variable while
      /// protecting it from potential modifications.
      /// Note that this mechanism relies on TLS and might not work with sandboxes.
      static size_t& counter()
      {
        static thread_local size_t _counter = 0;
        return _counter;
      }

      /// The _flag allows to completely disable preemption regardless of
      /// the counter value.
      /// _flag == true   -> preemptable.
      /// _flag == false  -> non-preemptable.
      ///
      /// Preemption is disabled at startup.
      static bool& flag()
      {
        static thread_local bool _flag = false;
        return _flag;
      }

  };
} //namespace verona::rt;

#endif
