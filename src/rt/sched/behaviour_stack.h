#pragma once


#include "threadstate.h"
#ifdef USE_PREEMPTION

#include <cstdint>
#include <cstdlib>
#include <sys/mman.h>
#include <ucontext.h>

typedef unsigned char _BYTE;

#define BEHAVIOUR_STACK_SIZE  0x5000 
#define SIGNAL_STACK_SIZE     0x2000


namespace verona::rt
{

  /// Assembly function to_behaviour to save current stack, load new one
  /// and call a switcher routine.
  __asm__(
    "\t.type to_behaviour,@function\n"
    "to_behaviour:\n"
    "\tmov %rsp, (%r8) #Save system stack.\n"
    "\tmov %rcx, %rsp  # Switch stack\n"
    "\tcall *%rdx # Call the __switcher; arguments are already in correct registers\n"
  );


  /// Assembly function to_system sets %eax to 0 to signify we were not preempted
  /// and switches back to the system stack to perform a return.
  __asm__(
    "\t.type to_system,@function\n"
    "to_system:\n"
    "\t mov $0x0, %eax # returned without preemption\n"
    "\tmov %rdi, %rsp # Stack pointer for the system, 1st argument\n"
    "\tret # Return on the system stack\n"
  );

  /// Switches to a behaviour stack. Arguments are:
  /// fn, behaviour, _switcher, stacks.behaviour, &stacks.system
  extern "C" bool to_behaviour(
      fncast, void*,
      void (*) (void(* )(void*), void*), _BYTE*, _BYTE**);
  extern "C" void to_system(_BYTE* system_stack);

  /// Convenience struct holding pointers to the system and behaviour stacks.
  struct ThreadStacks {
    _BYTE* system;
    _BYTE* behaviour;

    static ThreadStacks& get()
    {
      static thread_local ThreadStacks  stacks = {nullptr, nullptr};
      return stacks;
    }
  }; 

  /// This structure represents a behaviour's stack.
  /// It stores information about the current behaviour at the beginning 
  /// (lower addresses) of the stack.
  ///
  /// BehaviourStacks should be allocated using the allocate_stack function
  /// below.
  struct BehaviourStack
  {
    /// Encodes whether this is a preempted behaviour's stack.
    uint64_t type;

    /// Premmpted behaviour information.
    _BYTE* saved_stack;
    _BYTE* cown;
    _BYTE* message;
    ucontext_t context;
    uint64_t limit;
    _BYTE stack_bottom;

    /// The rest of the structure is for the stack.
    
    /// Mmaps a BEHAVIOUR_STACK_SIZE long memory region and casts it
    /// into a BehaviourStack pointer.
    static BehaviourStack* allocate_stack()
    {
      _BYTE* alloc = (_BYTE*) mmap(NULL, BEHAVIOUR_STACK_SIZE, PROT_READ|PROT_WRITE,
          MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
      if (alloc == MAP_FAILED)
        abort();
      return (BehaviourStack*)alloc;
    }

    /// MMap a stack for a scheduler's signal stack.
    static _BYTE* allocate_signal_stack()
    {
      _BYTE* stack = (_BYTE*) mmap(NULL, SIGNAL_STACK_SIZE, PROT_READ|PROT_WRITE,
          MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
      if (stack == MAP_FAILED)
        abort();
      return stack;
    }

    /// Unmaps a stack.
    static void deallocate_stack(BehaviourStack* stack)
    {
      if (stack == nullptr)
        abort();
      if (munmap(stack, BEHAVIOUR_STACK_SIZE) == -1)
        abort();
    }

    /// Unmaps a stack.
    static void deallocate_signal_stack(_BYTE* stack)
    {
      if (stack == nullptr)
        abort();
      if (munmap(stack, SIGNAL_STACK_SIZE) == -1)
        abort();
    }

    /// Returns a BehaviourStack pointer from a stack top.
    static BehaviourStack* from_top(_BYTE* top)
    {
      return (BehaviourStack*)(top - BEHAVIOUR_STACK_SIZE);
    }

    /// Returns the top of the BehaviourStack (higher address).
    _BYTE* top()
    {
      return ((_BYTE*)this)+BEHAVIOUR_STACK_SIZE;
    }


    /// Switching functions.
    static bool switch_to_behaviour(fncast fn, void* behaviour)
    {
      ThreadStacks& stacks = ThreadStacks::get();
      if (stacks.behaviour == nullptr)
        abort();
      return to_behaviour(fn, behaviour, _switcher, stacks.behaviour, &stacks.system);
    }

  private:
    // @Warning this function never returns.
    static void _switcher(fncast fn, void* behaviour)
    {
      /// Enable preemption now.
      Preempt::reenable_preemption();
      fn(behaviour);
      Preempt::disable_preemption();
      ThreadStacks& stacks = ThreadStacks::get();
      if (stacks.system == nullptr)
        abort();
      to_system(stacks.system);

      /// Should never return.
      abort();
    }
  };
} // namespace verona::rt;

#endif
