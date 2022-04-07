#pragma once

#include <atomic>

namespace verona::rt
{

  using atomic_counter = std::atomic<std::size_t>;
  class MonitorInfo {
    public:
      std::size_t size;
      atomic_counter *per_core_counters;
      std::atomic<bool> done;
    
    private:
      static MonitorInfo *instance;

    public:
      static MonitorInfo& get() {
        static MonitorInfo instance;
        return instance;
      }
    private:
      MonitorInfo() {
        size = 0;
        done = false;
      }
    public:
      MonitorInfo(MonitorInfo const&) = delete;
      void operator=(MonitorInfo const&) = delete;

  };
} // namespace verona::rt
