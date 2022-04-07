// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include "threading.h"
#include "sysmonitor.h"

#include <list>
#include <chrono>

/**
 * This constructs a platforms affinitised set of threads.
 */
namespace verona::rt
{
  class ThreadPoolBuilder
  {
    inline static Singleton<Topology, &Topology::init> topology;
    std::list<PlatformThread> threads;

    /// System monitor thread
    PlatformThread* sysmonitor = nullptr;

    size_t thread_count;
    size_t index = 0;

    template<typename... Args>
    void add_thread_impl(void (*body)(Args...), Args... args)
    {
      if (index < thread_count)
      {
        threads.emplace_back(body, args...);
      }
      else
      {
        Systematic::start();
        body(args...);
      }
    }

    template<typename... Args>
    static void
    run_with_affinity(size_t affinity, void (*body)(Args...), Args... args)
    {
      cpu::set_affinity(affinity);
      body(args...);
    }

    void monitor()
    {
      using namespace std::chrono_literals;
      MonitorInfo* moninfo = MonitorInfo::get();
      while(true)
      {
        std::this_thread::sleep_for(10ms);
        if (moninfo->done) {
          return;
        }

        // TODO check progress
        // TODO if no progress, spawn new thread with affinity to that core.
      }
    }

  public:
    ThreadPoolBuilder(size_t thread_count)
    {
      this->thread_count = thread_count - 1;

      // Bookkeeping information for the sysmonitor thread.
      MonitorInfo* moninfo = MonitorInfo::get();
      moninfo->done = false;
      moninfo->size = thread_count;
      moninfo->per_core_counters = (atomic_counter*)calloc(thread_count, sizeof(atomic_counter));
      for (size_t i = 0; i < moninfo->size; i++)
      {
        moninfo->per_core_counters[i] = 0;
      }
    }

    /**
     * Add a thread to run in this thread pool.
     */
    template<typename... Args>
    void add_thread(void (*body)(Args...), Args... args)
    {
#ifdef USE_SYSTEMATIC_TESTING
      // Don't use affinity with systematic testing.  We're only ever running
      // one thread at a time in systematic testing mode and by pinning each
      // thread to a core we massively increase contention.
      add_thread_impl(body, args...);
#else
      add_thread_impl(
        &run_with_affinity, topology.get().get(index), body, args...);
#endif
      index++;
    }

    size_t getIndex()
    {
      return index;
    }

    size_t getAffinity(size_t idx)
    {
      return topology.get().get(idx);
    }

    /**
     * The destructor waits for all threads to finish, and
     * then tidies up.
     *
     *  The number of executions is one larger than the number of threads
     * created as there is also the main thread.
     */
    ~ThreadPoolBuilder()
    {
      assert(index == thread_count + 1);
      sysmonitor = new PlatformThread(run_sysmonitor, this);
      MonitorInfo* moninfo = MonitorInfo::get();
      while (!threads.empty())
      {
        auto& thread = threads.front();
        thread.join();
        threads.pop_front();
      }
      moninfo->done = true;
      sysmonitor->join();
    }

    static void run_sysmonitor(ThreadPoolBuilder* builder)
    {
      builder->monitor();
    }
  };
}
