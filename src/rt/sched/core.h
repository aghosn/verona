#pragma once

#include <atomic>

#include "ds/mpscq.h"

#include "sched/threadpool.h"

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

      ~Core() {}

      void incrementServed()
      {
        progress_counter++;
      }
  };
} // namespace verona::rt
