#pragma once

#include <atomic>

#include "ds/mpscq.h"

#include "sched/threadpool.h"

namespace verona::rt
{
  
  template<class S, class T>
  class Core
  {
    public:
      size_t affinity = 0;
    private:
      friend ThreadPool<S>;
      friend S;
      friend T;

      MPMCQ<T> q;
      std::atomic<Core<S, T>*> next = nullptr;

    public:
      Core(T* token_cown) : q{token_cown}
      {}

      ~Core() {}
  };
} // namespace verona::rt
