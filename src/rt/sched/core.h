#pragma once

#include <atomic>

#include "ds/hashmap.h"
#include "ds/mpscq.h"
#include "mpmcq.h"

namespace verona::rt
{
  
  template<class T>
  class Core
  {
    std::atomic<std::size_t> progress_counter;
    public:
      size_t affinity = 0;
      MPMCQ<T> q;
      std::atomic<Core<T>*> next = nullptr;

    public:
      Core(T* token_cown) : q{token_cown}
      {}

      Core() {}

      ~Core() {}

      void incrementServed()
      {
        progress_counter++;
      }

      inline void schedule_lifo(T* a)
      {
        //TODO some logging + update stats.
        //I think we will need to put stats in here.
        q.enqueue_front(ThreadAlloc::get(), a);
      }
  };
} // namespace verona::rt
