#pragma once

#include "mpmcq.h"
#include "threadpool.h"
#include "workerthread.h"
#include "core.h"

namespace verona::rt
{
  
  template<class T>
  class Core 
  {
    public:
      private:
      friend T;
      MPMCQ<T> q;
      WorkerThread<Core<T>, T>* first_thread = nullptr; 
  };

} // namespace verona::rt
