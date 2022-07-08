#pragma once

#include "sched/stack.h"

namespace verona::rt
{

#ifdef USE_PREEMPTION
#define NO_PREEMPT() verona::rt::Preempt _nopreempt;
#else
#define NO_PREEMPT() {}
#endif

// TODO(aghosn) should disable preemption for both of these functions.
// Calling convention is rdi, rsi, rdx, rcx, r8, r9 
// Should protect that with x86_64 define
__asm__(
  "\t.type to_behaviour,@function\n"
  "to_behaviour:\n"
  "\tmov %rsp, (%r8) #Save system stack.\n"
  "\tmov %rcx, %rsp  # Switch stack\n"
  "\tcall *%rdx # Call the __switcher; arguments are already in correct registers\n"
);

__asm__(
  "\t.type to_system,@function\n"
  "to_system:\n"
  "\t mov $0x0, %eax # returned without preemption\n"
  "\tmov %rdi, %rsp # Stack pointer for the system, 1st argument\n"
  "\tret # Return on the system stack\n"
);

__asm__(
  "\t.type trampoline_preempt,@function\n"
  "trampoline_preempt:\n"
  "\tret # just return\n"
);


typedef struct {
  byte* system_stack;
  byte* behaviour_stack;
} ThreadStacks;

extern "C" bool to_behaviour(void (*target)(void*), void*, void (*) (void(* )(void*), void*), byte* behaviour_stack, byte** system_stack);
extern "C" void to_system(byte* system_stack);
extern "C" void trampoline_preempt(void);

#define BEHAVIOUR_STACK_SIZE 0x5000
  class Preempt
  {

    private:
      inline static std::atomic<size_t> interrupts;
      inline static std::atomic<size_t> preemptions;
    public:
      Preempt()
      {
        counter()++;
      }

      ~Preempt()
      {
        size_t& c = counter();
        if (c == 0)
          abort();
        c--;
      }

      static bool preemptable()
      {
        return counter() == 0;
      }

      static void reset()
      {
        interrupts = 0;
        preemptions = 0;
      }

      static size_t get_interrupts()
      {
        return interrupts;
      }
      static size_t get_preemptions()
      {
        return preemptions;
      }
      static void inc_interrupts()
      {
        interrupts++;
      }
      static void inc_preemptions()
      {
        preemptions++;
      }
     
      static ThreadStacks& thread_stacks()
      {
        static thread_local ThreadStacks stacks = {nullptr, nullptr};
        return stacks;
      }

      static bool switch_to_behaviour(void (*fn)(void*), void* behaviour)
      {
        ThreadStacks& stacks = thread_stacks();
        if (stacks.behaviour_stack == nullptr)
          abort();
        return to_behaviour(fn, behaviour, _switcher, stacks.behaviour_stack, &stacks.system_stack); 
      }

    private:

      // @warning This function never returns.
      static void _switcher(void (*fn)(void*), void* behaviour)
      {
        // Execute the function
        fn(behaviour);
        ThreadStacks& stacks = thread_stacks();
        if(stacks.system_stack == nullptr)
          abort();
        to_system(stacks.system_stack);
        // Should never return
        abort();
      }

      static size_t& counter()
      {
        static thread_local size_t _counter = 0;
        return _counter;
      }
  };
}
