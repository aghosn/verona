#pragma once
#include "core.h"
//#include "cown.h"
#include "corepool.h"
#include "threadpool.h"
#include "test/logging.h"

#include <cassert>

namespace verona::rt
{
  // Forward declaration to avoid circular dependencies.
  class Cown;
  class Runtime
  {
    private:
      // The pool of cores managed by this runtime
      CorePool<Runtime, Cown> core_pool;

      // The pool of threads managed by this runtime.
      using Scheduler = ThreadPool<Runtime, Cown>;
      friend Scheduler;

      //TODO system monitor here with statistics held inside the CorePool;
    public:
      static Runtime& get()
      {
        static Runtime instance;
        return instance;
      }

      // init creates the core and thread pools.
      // By default, for the moment, we would like to have exactly one thread
      // per core.
      void init(size_t count)
      {
        static bool init = false;
        if (init || count == 0)
          abort();
        
        // Create cores
        core_pool.init(count);

        // Create the threads
        Scheduler::get().init(count); 

        // Assign a thread per core.
        for (size_t i = 0; i < count; i++)
        {
          auto* core = core_pool.cores[i]; 
          Scheduler::get().assign_thread_to_core(core);
        }

        //TODO system monitor stuff here;
      }

      ///TODO aghosn moved this here from threadpool. 
      //Need to implement all the logging.
      bool check_for_work()
      {
        for(auto* core: core_pool.cores)
        {
          assert(core != nullptr);
          Logging::cout() << "Checking for pending work on thread "
                        << core->affinity << Logging::endl;
          if (!core->q.nothing_old())
          {
            Logging::cout() << "Found pending work!" << Logging::endl;
            return true;
          }
        }
        Logging::cout() << "No pending work!" << Logging::endl;
        return false;
      }

      Core<Cown>* first_core()
      {
        if (core_pool.cores.size() == 0)
          abort();
        return core_pool.cores[0]; 
      }
  };

} // namespace verona::rt
