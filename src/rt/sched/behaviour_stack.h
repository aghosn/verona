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

#define MARKED_PREEMPTED 0xdeadbeef


namespace verona::rt
{

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

    /// Preempted behaviour information.
    _BYTE* saved;
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
  };
} // namespace verona::rt;

#endif
