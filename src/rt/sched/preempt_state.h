#pragma once

#ifdef USE_PREEMPTION

#include <cassert>

#include "sched/behaviour_stack.h"

namespace verona::rt
{

  /// This structure is a thread local variable obtained by calling the static
  /// get function below. It holds a reference to the current BehaviourStak
  /// as well as the scheduler thread context and a flag signaling whether
  /// a behaviour ran to completion.
  struct PreemptState
  {
    BehaviourStack* behav_stack;    // behaviour stack.
    ucontext_t sched_ctxt;  // scheduler context.
    bool done;

    static PreemptState& get()
    {
      static thread_local PreemptState bs = {nullptr, {}, false};
      return bs;
    }

    /// Creates a context for a given behaviour.
    /// The context invokes the wrapper function below with the function
    /// and the behaviour as arguments.
    /// This function executes on the scheduler's stack and must be called
    /// before swapcontext between the scheduler and behaviour. 
    static void initialize_behaviour(fncast fn, void* behaviour)
    {
      PreemptState& bs = get();
      assert(bs.behav_stack != nullptr);
      getcontext(&bs.behav_stack->context);
      bs.behav_stack->context.uc_stack.ss_sp = (_BYTE*) bs.behav_stack; 
      bs.behav_stack->context.uc_stack.ss_size = BEHAVIOUR_STACK_SIZE;
      bs.behav_stack->context.uc_stack.ss_flags = 0;
      makecontext(&bs.behav_stack->context, (void (*)()) wrapper, 2, fn, behaviour);
    }

    /// Reenables preemption before executing the function.
    /// Once the behaviour is done executing, we disable preemption and set 
    /// the done flag to true.
    /// This function executes on the behaviour's stack.
    static void wrapper(fncast fn, void* behaviour)
    {
      Preempt::reenable_preemption();
      fn(behaviour);
      Preempt::disable_preemption();
      PreemptState& bs = get();
      bs.done = true;
      setcontext(&bs.sched_ctxt);  
    }
  };


} // namespace verona::rt;

#endif
