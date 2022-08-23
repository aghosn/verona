#pragma once

/// SystemMonitor defines a thread that monitors the verona runtime's progress
/// on each core. If a stall is observed, it spawns or unparks extra scheduler
/// threads on that core to service cowns. The system monitor sacrifices
/// a core's throughput to prevent starvation/hogging of resources.
/// The implementation strives to return to a stable state with one scheduler
/// thread per core.
///
/// When preemption is enabled, the sysmonitor preempts long running behaviours
/// rather than spawning extra scheduler threads.
///
/// The MONITOR_QUANTUM variable is set at compile time.
#include "preempt.h"
#ifdef USE_SYSMONITOR

#include "pal/threadpoolbuilder.h"
#include "test/logging.h"
#include "pal/threading.h"

#include <atomic>
#include <chrono>
#include <cassert>

#include "sched/preempt.h"

#ifdef USE_PREEMPTION
#include <signal.h>
#include <pthread.h>

#include "sched/preempt_state.h"
#include "sched/behaviour_stack.h"

/// The signal used for preemption.
#define SIG_PREEMPT SIGUSR1
#endif


namespace verona::rt
{
  // Singleton class monitoring the progress on cores.
  template<class Scheduler>
  class SysMonitor
  {
    private:
      friend Scheduler;
      /// When true, the SysMonitor should stop.
      std::atomic_bool done = false;

#ifdef USE_SYSTEMATIC_TESTING
    friend class ThreadSyncSystematic<SysMonitor<Scheduler>>;
    Systematic::Local* local_systematic{nullptr};
#endif
      SysMonitor() {}
    
    public:
      SysMonitor(SysMonitor const&) = delete;

      static SysMonitor<Scheduler>& get()
      {
        NO_PREEMPT();
       static SysMonitor<Scheduler> instance;
        return instance;
      }

      void run_monitor(ThreadPoolBuilder& builder)
      {
        NO_PREEMPT();
#ifdef USE_SYSTEMATIC_TESTING
        Systematic::start();
        Systematic::attach_systematic_thread(this->local_systematic);
#endif 
        auto* pool = Scheduler::get().core_pool;
        assert(pool != nullptr);
        assert(pool->core_count != 0);

        while(!done)
        {
          size_t scan[pool->core_count]; 
          for (size_t i = 0; i < pool->core_count; i++)
          {
            scan[i] = pool->cores[i]->progress_counter;
          }
#ifdef USE_SYSTEMATIC_TESTING
          size_t start = 0;
          while(start < 1000)
          {
            yield();
            start++;
          }
#else
          std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_QUANTUM));
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

#ifndef USE_PREEMPTION
              // We pass the count as argument in case there was some progress
              // in the meantime.
              Scheduler::get().spawnThread(builder, pool->cores[i], count);
#else
              // TODO send a signal to the thread.
              UNUSED(builder);
#ifndef USE_SYSTEMATIC_TESTING
              /// Send the signal to the thread.
              if (pool->cores[i]->thread_valid) {
                Preempt::preempted_interrupts++;
                pthread_kill(pool->cores[i]->thread, SIG_PREEMPT);
              }
#endif
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
        Systematic::finished_thread();
      }

      void threadExit()
      {
        NO_PREEMPT();
        Logging::cout() << "Thread exit: setting done to true" << Logging::endl;
        // if a thread exits, we are done
        done = true;
      }

    private:
    /// Preemption functions.
#ifdef USE_PREEMPTION
      static void signal_handler(int sig, siginfo_t* info, void* _context)
      {
        bool preemptable = Preempt::is_preemptable();
        // Disable preemption before returning.
        Preempt::disable_preemption();

        UNUSED(info);
        assert(sig == SIG_PREEMPT);

        ucontext_t* context = (ucontext_t*)_context;

        /// We are unable to proceed with preemption if we do not have a context.
        if (context == nullptr)
          abort();

        /// Make sure we have a local scheduler thread set.
        auto* t = Scheduler::get().local();
        if (t == nullptr)
          abort();

        /// The thread is in a NO_PREEMPT block.
        if (!preemptable)
          return;

        /// The thread is preemptable.
        PreemptState& state = PreemptState::get();
        if (state.behav_stack == nullptr)
          abort();
        greg_t rsp = context->uc_mcontext.gregs[REG_RSP];
        BehaviourStack* bstack = state.behav_stack;  

        /// Check that we did not overwrite the content of the stack in the lower
        /// addresses.
        if (rsp <= (greg_t)(&bstack->limit))
          abort();

        Preempt::preempted_count++;
        sigemptyset(&context->uc_sigmask);
        swapcontext(&bstack->context, &state.sched_ctxt); 
      }

      /// Register the handler for preemption's signal.
      /// We use SIGUSR1.
      static void init_preemption()
      {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sa.sa_sigaction = signal_handler;
        sigaction(SIG_PREEMPT, &sa, NULL);

        Preempt::preempted_count = 0;
        Preempt::preempted_interrupts = 0;
      }
#endif
  };
}
#endif
