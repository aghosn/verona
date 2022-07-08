#pragma once

#include <sys/mman.h>
#include <ucontext.h>

#define BEHAVIOUR_STACK_SIZE 0x5000

typedef unsigned char byte;

#define MARKED_PREEMPTED 0x666

namespace verona::rt
{

  // A type that represents a stack.
  // Allocation should happen using the allocate_stack function below.
  // Type allows to encode whether the behaviour was preempted
  // TODO add cown pointer too
  struct BehaviourStack
  {
    uint64_t type;
    byte* saved_stack;
    byte* cown;
    byte* message;
    ucontext_t context; 
    uint64_t limit;
    byte stack_bottom;
    /// The rest is for the stack

    byte* get_top()
    {
      return ((byte*)(this)) + BEHAVIOUR_STACK_SIZE;
    }

    static BehaviourStack* stack_from_top(byte* top)
    {
      return (BehaviourStack*)(top - BEHAVIOUR_STACK_SIZE);
    }

    static BehaviourStack* allocate_stack()
    {
      byte* alloc = (byte*) mmap(NULL, BEHAVIOUR_STACK_SIZE, PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
      if (alloc == MAP_FAILED)
        abort();
      return (BehaviourStack*)(alloc);
    }
    
    static void deallocate_stack(BehaviourStack* stack)
    {
      if (stack != nullptr && munmap(stack, BEHAVIOUR_STACK_SIZE) == -1)
        abort();
    }
  };
}
