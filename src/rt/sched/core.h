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
  private:
    std::atomic<std::size_t> progress_counter;
  public:
    size_t affinity = 0;
    T* token_cown = nullptr;
    MPMCQ<T> q;
    std::atomic<Core<T>*> next = nullptr;

    /// Moved from the scheduler thread
    size_t total_cowns = 0;
    std::atomic<size_t> free_cowns = 0;


  public:
    Core() : token_cown{T::create_token_cown()}, q{token_cown}
    {
      token_cown->set_owning_core(this);
    }

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
