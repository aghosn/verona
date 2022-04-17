#pragma once

#include <atomic>

#include "ds/mpscq.h"

#include "sched/threadpool.h"

namespace verona::rt
{
  
  template<class T>
  class Core
  {
    public:
      size_t affinity = 0;
      MPMCQ<T> q;
      std::atomic<Core<T>*> next = nullptr;
    public:
      Core(T* token_cown) : q{token_cown}
      {}

      ~Core() {}
  };
} // namespace verona::rt
