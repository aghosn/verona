// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include "threading.h"

#include <list>
#include <mutex>
#include <cassert>

/**
 * This constructs a platforms affinitised set of threads.
 */
namespace verona::rt
{
  template<class T>
  class ThreadPoolBuilder
  {
  private:
    T* pool = nullptr;
    std::list<PlatformThread> threads;

    template<typename... Args>
    void add_thread_impl(void (*body)(Args...), Args... args)
    {
      threads.emplace_back(body, args...);
    }

    template<typename... Args>
    static void
    run_with_affinity(size_t affinity, void (*body)(Args...), Args... args)
    {
      cpu::set_affinity(affinity);
      body(args...);
    }

  public:
    ThreadPoolBuilder(T* pool) : pool(pool)
    {
      if (pool == nullptr)
        abort();
    }

    ~ThreadPoolBuilder()
    {
      // The constructor acquires the lock.
      /*std::unique_lock lk(pool->mut);
      // Required loop due to spurious wake-ups.
      while(pool->active_thread_count > 0)
      {
        pool->cv.wait(lk, [this]{return pool->active_thread_count == 0;});
      }
      lk.unlock();

      // TODO figure out if we should wake up all threads
      pool->flag_done = true;*/

      while(!threads.empty())
      {
        auto& thread = threads.front();
        thread.join();
        threads.pop_front();
      }
      /// Debugging checks
      //assert(pool->active_thread_count == 0);
    }

    template<typename... Args>
    void add_thread(size_t index, void (*body)(Args...), Args... args)
    {
      //TODO do the systematic thing.
      //TODO need to replace this with another counter
      //pool->active_thread_count++;
      //threads.emplace_back(body, args...);
      //pool->active_thread_count--;
      //pool->cv.notify_one();
#ifdef USE_SYSTEMATIC_TESTING
      // Don't use affinity with systematic testing.  We're only ever running
      // one thread at a time in systematic testing mode and by pinning each
      // thread to a core we massively increase contention.
      UNUSED(index);
      add_thread_impl(body, args...);
#else
    add_thread_impl(
        &run_with_affinity, index, body, args...);
#endif
    }

  };
}
