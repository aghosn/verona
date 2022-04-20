#pragma once

#include <atomic>

namespace verona::rt
{
  
  template<class T>
  class Core
  {
    private:
      friend T;

      using atomic_counter = std::atomic<std::size_t>;

      // Bookkeeping information about the core
      atomic_counter progress_counter = 0;
      // Number of threads servicing this core;
      atomic_counter nb_threads = 0; 

    public:
      size_t affinity = 0;
      //TODO MPMCQ<T> q; 
      //TODO token_cown;
      Core<T>* next = nullptr;

      Core() {}
      ~Core() {}
  };
} // namespace verona::rt
