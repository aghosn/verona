#pragma once
#include "core.h"
#include "cown.h"
#include "corepool.h"
#include "threadpool.h"

namespace verona::rt
{
  class Runtime
  {
    private:
      // The pool of cores managed by this runtime
      CorePool<Runtime, Cown> core_pool;

      // The pool of threads managed by this runtime.
      ThreadPool<Cown> thread_pool;

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
        thread_pool.init(count); 

        // Assign a thread per core.
        for (size_t i = 0; i < count; i++)
        {
          auto* core = core_pool.cores[i]; 
          thread_pool.assign_thread_to_core(core);
        }

        //TODO system monitor stuff here;

        /// Let's run now.
        thread_pool.run();
      }
  };

} // namespace verona::rt
