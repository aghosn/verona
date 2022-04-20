#pragma once

/*#ifdef USE_SYSTEMATIC_TESTING
#  include "sched/threadsyncsystematic.h"
#else
#  include "sched/threadsync.h"
#endif */

#include <list>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "schedulerthread.h"
#include "threadpoolbuilder.h"

namespace verona::rt
{
  /// Used for default prerun for a thread.
  inline void nop() {}

  template<class T>
  class ThreadPool
  {
    private:
      using WorkerThread = SchedulerThread<ThreadPool<T>, T>;
      friend ThreadPoolBuilder<ThreadPool>;
      friend WorkerThread;
      friend void verona::rt::yield();

      using atomic_counter = std::atomic<std::size_t>;

      static constexpr uint64_t TSC_PAUSE_SLOP = 1'000'000;
      static constexpr uint64_t TSC_UNPAUSE_SLOP = TSC_PAUSE_SLOP / 2;

      bool detect_leaks = true;
      size_t incarnation = 1;

      /**
      * Number of messages that have been sent that may not be visible to a
      *thread in a Scan state.
      **/
      std::atomic<size_t> inflight_count = 0;

      /**
      * Used to represent the current pause_epoch.
      *
      * If a thread is paused, then it must be the case
      * that pause_epoch is ahead of unpause_epoch.
      */
      std::atomic<uint64_t> pause_epoch{0};

      /**
      * Used to track unpause calls.  Threads unpausing
      * attempt to catch unpause_epoch up to pause_epoch,
      * and thus ensure threads are running.
      */
      std::atomic<uint64_t> unpause_epoch{0};

/*#ifdef USE_SYSTEMATIC_TESTING
      ThreadSyncSystematic<WorkerThread> sync;
#else
      ThreadSync<WorkerThread> sync;
#endif */

      std::atomic_uint64_t barrier_count = 0;
      uint64_t barrier_incarnation = 0;

      /// How many threads are being managed by this pool
      atomic_counter thread_count = 0;
      
      /// How many threads are not currently paused.
      atomic_counter active_thread_count = 0;

      /// Count of external event sources, such as I/O, that will prevent
      /// quiescence.
      size_t external_event_sources = 0;

      bool teardown_in_progress = false;

      bool fair = false;

      ThreadState state;

      //TODO Probably will need these two to be atomic/locked?
      std::list<WorkerThread*> free_pool;
      std::list<WorkerThread*> active_pool;

      /// Internal state to synchronize threads and allow graceful termination
      std::atomic<bool> flag_done = false;
      std::mutex mut;
      std::condition_variable cv;
   
    public:
      void set_detect_leaks(bool b)
      {
        detect_leaks = b;
      }

      bool get_detect_leaks()
      {
        return detect_leaks;
      }

      void record_inflight_message()
      {
        Logging::cout() << "Increase inflight count: " << inflight_count + 1
                      << Logging::endl;
        local()->scheduled_unscanned_cown = true;
        inflight_count++;
      }

      void recv_inflight_message()
      {
        Logging::cout() << "Decrease inflight count: " << inflight_count - 1
                        << Logging::endl;
        inflight_count--;
      }

      bool no_inflight_messages()
      {
        Logging::cout() << "Check inflight count: " << inflight_count
                        << Logging::endl;
        return inflight_count == 0;
      }

      /// Increment the external event source count. A non-zero count will prevent
      /// runtime teardown.
      /// This should only be called from inside the runtime.
      /// A message can be enqueued before the runtime is running if there is a
      /// external event source from the start.
      void add_external_event_source()
      {
        auto h = sync.handle(local());
        assert(local() != nullptr);
        auto prev_count = s.external_event_sources++;
        Logging::cout() << "Add external event source (now " << (prev_count + 1)
                        << ")" << Logging::endl;
      }

      void init(size_t count)
      {
        for(size_t i = 0; i < count; i++)
        {
          free_pool.emplace_back(new WorkerThread(this));
        }
      }

      void assign_thread_to_core(Core<T>* core)
      {
        if (core == nullptr)
          abort();
        WorkerThread* result = nullptr;
        if (free_pool.size() != 0)
        {
          result = free_pool.front();
          free_pool.pop_front();
        }
        else 
          result = new WorkerThread(this);
        result->set_core(core);
        /// Add the thread to the list of active threads
        active_pool.emplace_back(result);
      }

      void run()
      {
        run_with_startup<>(&nop); 
      }

      template<typename... Args>
      void run_with_startup(void (*startup)(Args...), Args... args)
      {
        {
           ThreadPoolBuilder<ThreadPool> builder(this);
          for (auto* thread: active_pool)
          {
            // All active threads should have been assigned a core by now.
            if (thread->core == nullptr)
              abort();
            builder.add_thread(&WorkerThread::run, thread, startup, args...);
          }
          /// ThreadPoolBuilder will be destroyed here.
        } 

        //TODO cleanup
      }
  };

} // namespace verona::rt
