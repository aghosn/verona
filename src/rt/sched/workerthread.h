#pragma once

#include "threadpool.h"
#include "core.h"

namespace verona::rt
{
  
  template<class C, class T>
  class WorkerThread
  {
    public:
      /// Friendly thread identifier for logging information
      size_t systematic_id = 0;
    private:
      using Scheduler = ThreadPool<C>;
      friend T;
      friend C;

      /// Reference to the current core we are servicing
      C* core = nullptr;
      C* victim = nullptr;

      bool running = true;

      bool should_steal_for_fairness = false;
      
      WorkerThread()
      {}

      ~WorkerThread() {}

      inline void stop()
      {
        running = false;
      }

      template<typename... Args>
      static void run(C* t, void (*startup)(Args...), Args... args)
      {
        t->run_inner(startup, args...);
      } 
  
    /**
     * Startup is supplied to initialise thread local state before the runtime
     * starts.
     *
     * This is used for initialising the interpreters per-thread data-structures
     **/
    template<typename... Args>
    void run_inner(void (*startup)(Args...), Args... args)
    {
      startup(args...);

      Scheduler::local() = core;//this;
      core->alloc = &ThreadAlloc::get();
      victim = core->next;
      T* cown = nullptr;

#ifdef USE_SYSTEMATIC_TESTING
      Systematic::attach_systematic_thread(this->local_systematic);
#endif

      while (true)
      {
        if (
          (core->total_cowns < (core->free_cowns << 1))
#ifdef USE_SYSTEMATIC_TESTING
          || Systematic::coin()
#endif
        )
          core->collect_cown_stubs();

        if (should_steal_for_fairness)
        {
          if (cown == nullptr)
          {
            should_steal_for_fairness = false;
            fast_steal(cown);
          }
        }

        if (cown == nullptr)
        {
          cown = core->q.dequeue(*core->alloc);
          if (cown != nullptr)
            Logging::cout()
              << "Pop cown " << core->clear_thread_bit(cown) << Logging::endl;
        }

        if (cown == nullptr)
        {
          cown = core->steal();

          // If we can't steal, we are done.
          if (cown == nullptr)
            break;
        }

        // Administrative work before handling messages.
        if (!core->prerun(cown))
        {
          cown = nullptr;
          continue;
        }

        Logging::cout() << "Schedule cown " << cown << " ("
                        << cown->get_epoch_mark() << ")" << Logging::endl;

        // This prevents the LD protocol advancing if this cown has not been
        // scanned. This catches various cases where we have stolen, or
        // reschedule with the empty queue. We are effectively rescheduling, so
        // check if unscanned. This seems a little agressive, but prevents the
        // protocol advancing too quickly.
        // TODO refactor this could be made more optimal if we only do this for
        // stealing, and running on same cown as previous loop.
        if (Scheduler::should_scan() && (cown->get_epoch_mark() != core->send_epoch))
        {
          Logging::cout() << "Unscanned cown next" << Logging::endl;
          core->scheduled_unscanned_cown = true;
        }

        core->ld_protocol();

        Logging::cout() << "Running cown " << cown << Logging::endl;

        /// Increment the number of served behaviours
        MonitorInfo::incrementServed(core->affinity);

        bool reschedule = cown->run(*core->alloc, core->state);

        if (reschedule)
        {
          if (should_steal_for_fairness)
          {
            schedule_fifo(cown);
            cown = nullptr;
          }
          else
          {
            assert(!cown->queue.is_sleeping());
            // Push to the back of the queue if the queue is not empty,
            // otherwise run this cown again. Don't push to the queue
            // immediately to avoid another thread stealing our only cown.

            T* n = core->q.dequeue(*core->alloc);

            if (n != nullptr)
            {
              core->schedule_fifo(cown);
              cown = n;
            }
            else
            {
              if (core->q.nothing_old())
              {
                Logging::cout() << "Queue empty" << Logging::endl;
                // We have effectively reached token cown.
                core->n_ld_tokens = 0;

                T* stolen;
                if (Scheduler::get().fair && core->fast_steal(stolen))
                {
                  core->schedule_fifo(cown);
                  cown = stolen;
                }
              }

              if (!core->has_thread_bit(cown))
              {
                Logging::cout()
                  << "Reschedule cown " << cown << " ("
                  << cown->get_epoch_mark() << ")" << Logging::endl;
              }
            }
          }
        }
        else
        {
          // Don't reschedule.
          cown = nullptr;
        }

        yield();
      }

      Logging::cout() << "Begin teardown (phase 1)" << Logging::endl;

      cown = core->list;
      while (cown != nullptr)
      {
        if (!cown->is_collected())
          cown->collect(*core->alloc);
        cown = cown->next;
      }

      Logging::cout() << "End teardown (phase 1)" << Logging::endl;

      Epoch(ThreadAlloc::get()).flush_local();
      Scheduler::get().enter_barrier();

      Logging::cout() << "Begin teardown (phase 2)" << Logging::endl;

      GlobalEpoch::advance();

      core->template collect_cown_stubs<true>();

      Logging::cout() << "End teardown (phase 2)" << Logging::endl;

      core->q.destroy(*(core->alloc));

      Systematic::finished_thread();

      // Reset the local thread pointer as this physical thread could be reused
      // for a different Core later.
      Scheduler::local() = nullptr;

      MonitorInfo::threadExit();
    }

  };

} // namespace verona::rt
