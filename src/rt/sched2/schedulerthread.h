#pragma once

#include <atomic>

#include "core.h"
#include "object/object.h"
#include "sched/threadstate.h"
#include "sched/schedulerstats.h"

namespace verona::rt
{
  template<class P, class T>
  class SchedulerThread
  {
    public:
      /// Friendly thread identifier for logging information.
      size_t systematic_id = 0;
    private:
      friend P;

      P* pool = nullptr;

      static constexpr uint64_t TSC_QUIESCENCE_TIMEOUT = 1'000'000;

      T* token_cown = nullptr;

      Core<T>* core = nullptr;
      Core<T>* victim = nullptr;
      Alloc* alloc = nullptr;
      
      bool running = true;

      // `n_ld_tokens` indicates the times of token cown a scheduler has to
      // process before reaching its LD checkpoint (`n_ld_tokens == 0`).
      uint8_t n_ld_tokens = 0;

      bool should_steal_for_fairness = false;

      std::atomic<bool> scheduled_unscanned_cown = false;

      EpochMark send_epoch = EpochMark::EPOCH_A;
      EpochMark prev_epoch = EpochMark::EPOCH_B;

      ThreadState::State state = ThreadState::State::NotInLD;
      SchedulerStats stats;

      T* list = nullptr;
      size_t total_cowns = 0;
      std::atomic<size_t> free_cowns = 0;

    public:
      SchedulerThread(P *pool) : pool(pool)
      {
        //TODO do we have anything to do here?
      }

      ~SchedulerThread() {}

      inline void stop()
      {
        running = false;
      }

      inline void schedule_fifo(T* a)
      {
        Logging::cout() << "Enqueue cown " << a << " (" << a->get_epoch_mark()
                        << ")" << Logging::endl;

        // Scheduling on this thread, from this thread.
        if (!a->scanned(send_epoch))
        {
          Logging::cout() << "Enqueue unscanned cown " << a << Logging::endl;
          scheduled_unscanned_cown = true;
        }
        assert(!a->queue.is_sleeping());
        core->q.enqueue(*alloc, a);

        if (pool->unpause())
          stats.unpause();
      }

      void set_core(Core<T>* c)
      {
        /// This thread is already pinned to a core or the supplied one is null
        if (core != nullptr || c == nullptr)
          abort();

        core = c;
        victim = core->next;
      }

      template<typename... Args>
      static void run(SchedulerThread* t, void(*startup)(Args...), Args... args)
      {
        t->run_inner(startup, args...);
      }

      template<typename... Args>
      void run_inner(void (*startup)(Args...), Args... args)
      {
        startup(args...);
        //TODO 
      }
      
  };
} // namespace verona::rt
