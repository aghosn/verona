#pragma once

#include "behaviour.h"

namespace verona::rt
{
  class Cown;
  template<class T>
  class PreemptedBehaviour : public Behaviour
  {
    friend class Cown;
    private:
      T fn;

      static void f(Behaviour* msg)
      {
        auto t = static_cast<PreemptedBehaviour<T>*>(msg);
        t->fn();

        if constexpr (!std::is_trivially_destructible_v<PreemptedBehaviour<T>>)
        {
          t->~PreemptedBehaviour<T>();
        }
      }

      static const Behaviour::Descriptor* desc()
      {
        static constexpr Behaviour::Descriptor desc = {
          sizeof(PreemptedBehaviour<T>), f, NULL};
        return &desc;
      }

      void* operator new(size_t, PreemptedBehaviour* obj)
      {
        return obj;
      }
    public:
      PreemptedBehaviour(T fn_) : Behaviour(desc()), fn(std::move(fn_))
      {}
  };

}
