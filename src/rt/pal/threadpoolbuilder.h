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

    /*template<typename... Args>
    void add_thread_impl(bool skip, void (*body)(Args...), Args... args)
    {
      if (skip || index < thread_count)
      {
        threads.emplace_back(body, args...);
      }
      else
      {
        Systematic::start();
        body(args...);
      }
    }*/

    /*template<typename... Args>
    static void
    run_with_affinity(size_t affinity, void (*body)(Args...), Args... args)
    {
      cpu::set_affinity(affinity);
      body(args...);
    }*/

  public:
    ThreadPoolBuilder(T* pool) : pool(pool)
    {
      if (pool == nullptr)
        abort();
    }

    ~ThreadPoolBuilder()
    {
      // The constructor acquires the lock.
      std::unique_lock lk(pool->mut);
      // Required loop due to spurious wake-ups.
      while(pool->active_thread_count > 0)
      {
        pool->cv.wait(lk, [this]{return pool->active_thread_count == 0;});
      }
      lk.unlock();

      // TODO figure out if we should wake up all threads
      pool->flag_done = true;

      while(!threads.empty())
      {
        auto& thread = threads.front();
        thread.join();
        threads.pop_front();
      }
      /// Debugging checks
      assert(pool->active_thread_count == 0);
    }

    template<typename... Args>
    void add_thread(void (*body)(Args...), Args... args)
    {
      //TODO do the systematic thing.
      pool->active_thread_count++;
      threads.emplace_back(body, args...);
      pool->active_thread_count--;
      pool->cv.notify_one();
    }

  };
}
