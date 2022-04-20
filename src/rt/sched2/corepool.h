#pragma once

#include <vector>
#include "core.h"
#include "pal/threading.h"

namespace verona::rt
{
  // Expected to be instantiated with a cown.
  // This is meant to be a singleton.
  template<class P, class T>
  class CorePool
  {
    private:
      friend P;

      inline static Singleton<Topology, &Topology::init> topology;
      Core<T>* first_core = nullptr; 
      size_t core_count = 0;
      std::vector<Core<T>*> cores;
      
    public:
      CorePool()
      {

      }

      ~CorePool()
      {
        //TODO collect everything
      }

      void init(size_t count)
      {
        core_count = count;
        first_core = new Core<T>;
        Core<T>* t = first_core;
        cores.emplace_back(first_core);

        while (true)
        {
          t->affinity = topology.get().get(count);

          if (count > 1)
          {
            t->next = new Core<T>;
            t = t->next;
            count--;
            cores.emplace_back(t);
          }
          else
          {
            t->next = first_core;
            break;
          }
        }
      }
  };

} // namespace verona::rt
