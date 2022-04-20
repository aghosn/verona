#pragma once
#include "core.h"
#include "cown.h"
#include "corepool.h"
#include "threadpool.h"

#include <cassert>

namespace verona::rt
{
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

        /// Let's run now.
        Scheduler::get().run();
      }

      ///TODO aghosn moved this here from threadpool. 
      //Need to implement all the logging.
      bool check_for_work()
      {
        for(auto* core: core_pool.cores)
        {
          assert(core != nullptr);
          //TODO fix this once we merge with core logic
          //Just trying to see if stuff compiles
          if (/*core->q.nothing_old()*/ true)
          {
            //TODO logging
            return true;
          }
        }
        //TODO logging
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
