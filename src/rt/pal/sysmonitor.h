#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace verona::rt
{

  using atomic_counter = std::atomic<std::size_t>;
  class MonitorInfo {
    public:
      std::size_t size;
      atomic_counter *per_core_counters;
      std::atomic<bool> done;
    
      // For gracefull termination, TODO implement
      std::mutex m;
      std::condition_variable cv;
      std::atomic<std::size_t> threads;
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
