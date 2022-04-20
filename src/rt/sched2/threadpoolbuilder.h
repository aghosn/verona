#pragma once

#include "pal/threading.h"

#include <list>
#include <mutex>
#include <cassert>

namespace verona::rt
{
  template<class T>
  class ThreadPoolBuilder
  {
    private:
      T* pool = nullptr;
      std::list<PlatformThread> threads;
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

} // namespace verona::rt
