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
    
      // For gracefull termination 
      std::mutex m;
      std::condition_variable cv;
      std::atomic<std::size_t> threads;
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

      static void threadExit()
      {
        MonitorInfo::get().m.lock();
        MonitorInfo::get().threads--;
        MonitorInfo::get().m.unlock();
        MonitorInfo::get().cv.notify_one();
      }

      static void incrementServed(size_t affinity)
      {
        if (affinity >= MonitorInfo::get().size)
        {
          std::cerr << "Error: invalid affinity " << affinity << std::endl;
          abort();
        }
        if (MonitorInfo::get().per_core_counters[affinity] == SIZE_MAX-1)
        {
          MonitorInfo::get().per_core_counters[affinity] = 1;
          return;
        }
        MonitorInfo::get().per_core_counters[affinity]++;
      }
  };
} // namespace verona::rt
