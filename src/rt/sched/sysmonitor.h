#pragma once

#include "pal/threadpoolbuilder.h"
#include "test/logging.h"
#include "pal/threading.h"

#include <atomic>
#include <chrono>
#include <cassert>

#ifdef USE_PREEMPTION
#include <signal.h>
#include <pthread.h>
#include <iostream>
#include "preempt.h"
#endif

namespace verona::rt
{
  // Singleton class monitoring the progress on cores.
  // Whenever the SysMonitor detects a hogged core, it 
  // calls into the threadpool to schedule a new thread on that 
  // particular core.
  template<class Scheduler>
  class SysMonitor
  {
    private:
      friend Scheduler;
      /// When true, the SysMonitor should stop.
      std::atomic_bool done = false;
      std::atomic<size_t> preempt_count;
#ifdef USE_SYSTEMATIC_TESTING
    friend class ThreadSyncSystematic<SysMonitor<Scheduler>>;
    Systematic::Local* local_systematic{nullptr};
#endif
      SysMonitor() {}

#ifdef USE_PREEMPTION
    static void signal_handler(int sig, siginfo_t *info, void* _context)
    {
      UNUSED(sig);
      UNUSED(info);
      UNUSED(_context);
      Preempt::inc_interrupts();
      ucontext_t *context = (ucontext_t*)_context;
      if (context == nullptr)
        abort();
      get().preempt_count++;
      auto *t = Scheduler::get().local();
      if (t == nullptr)
        abort();
      if (!Preempt::preemptable()) {
        return;
      }
      Preempt::inc_preemptions();
      //abort();
    }
#endif
    
    public:
      SysMonitor(SysMonitor const&) = delete;

      static SysMonitor<Scheduler>& get()
      {
       static SysMonitor<Scheduler> instance;
        return instance;
      }

#ifdef USE_PREEMPTION
      // Register the handler for preemption signals
      void init_preemption()
      {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sa.sa_sigaction = signal_handler;
        sigaction(SIGUSR1, &sa, NULL);
        preempt_count = 0;
      }
#endif

      void run_monitor(ThreadPoolBuilder& builder)
      {
#ifdef USE_SYSTEMATIC_TESTING
        Systematic::start();
        Systematic::attach_systematic_thread(this->local_systematic);
#endif 
        using namespace std::chrono_literals;
        auto* pool = Scheduler::get().core_pool;
        assert(pool != nullptr);
        assert(pool->core_count != 0);

        // TODO expose this as a tunable parameter
        auto quantum = 10ms;
        while(!done)
        {
          size_t scan[pool->core_count]; 
          for (size_t i = 0; i < pool->core_count; i++)
          {
            scan[i] = pool->cores[i]->progress_counter;
          }
#ifdef USE_SYSTEMATIC_TESTING
          UNUSED(quantum);
          size_t start = 0;
          while(start < 1000)
          {
            yield();
            start++;
          }
#else
          std::this_thread::sleep_for(quantum);
#endif

          if (done)
          {
            Logging::cout() << "System monitor break" << Logging::endl;
            break;
          }
          // Look for progress 
          for (size_t i = 0; i < pool->core_count; i++)
          {
            size_t count = pool->cores[i]->progress_counter; 
            // Counter is the same and there is some work to do.
            if (scan[i] == count && !pool->cores[i]->q.nothing_old())
            {
              Logging::cout() << "System monitor detected lack of progress on core " << i << Logging::endl;
              // We pass the count as argument in case there was some progress
              // in the meantime.
#ifdef USE_PREEMPTION
             UNUSED(builder);
#ifndef USE_SYSTEMATIC_TESTING
            pthread_kill(pool->cores[i]->thread, SIGUSR1);
#endif
#else
              Scheduler::get().spawnThread(builder, pool->cores[i], count);
#endif
            }
          }
        }
        assert(done);
        // Wake all the threads stuck on their condition vars.
        Scheduler::get().wakeWorkers();
        // As this thread is the only one modifying the builder thread list,
        // we know nothing should be modifying it right now and can thus exit 
        // to join on every single thread.
        Logging::cout() << "System monitor exit" << Logging::endl;
#ifdef USE_PREEMPTION
        std::cout << "Preemption count " << preempt_count << std::endl;
#endif
        Systematic::finished_thread();
      }

      void threadExit()
      {
        Logging::cout() << "Thread exit: setting done to true" << Logging::endl;
        // if a thread exits, we are done
        done = true;
      }
  };
}
