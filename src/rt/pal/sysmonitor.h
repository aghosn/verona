#pragma once

#include <atomic>

namespace verona::rt
{

  using atomic_counter = std::atomic<size_t>;
  class MonitorInfo {
    public:
      size_t size;
      atomic_counter *per_core_counters;
      std::atomic<bool> done;
    
    private:
      static MonitorInfo *instance;

    public:
      static MonitorInfo* get() {
        static MonitorInfo* instance = nullptr;
        if (instance == nullptr)
          instance = new MonitorInfo();
        return instance;
      }
    private:
      MonitorInfo() {
        size = 0;
        done = false;
      }
  };
} // namespace verona::rt
