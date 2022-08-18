#include "args.h"

#include <cpp/when.h>
#include <atomic>
#include <test/harness.h>

using namespace verona::cpp;

std::atomic<size_t> counter = 0;

// Long running behavior that requires behaviors scheduled after it
// to make progress (i.e., not starve due to the core being hogged) in order
// to finish.
// This tests the system monitor's ability to spawn new scheduler threads.
void long_running()
{
  Logging::cout() << "Starting the long running" << std::endl;
  while(counter != 0) {
    Logging::cout() << "Long running about to yield " <<  counter << Logging::endl;
    yield();
  }
  Logging::cout() << "Long running behavior just finished" << std::endl;
}

void short_running()
{
  Logging::cout() << "Running short behavior" << std::endl;
  counter--;
}

void test_inner()
{
  // Set the counter to the number of short-lived behaviors.
  counter = NUM_SMALL;
  when() << long_running;
  for (size_t i = 0; i < NUM_SMALL; i++)
  {
    when() << short_running;
  }
}

// Required first `when` to make sure we enqueue fifo
void test1()
{
  when() << test_inner;
}


int verona_main(SystematicTestHarness& harness)
{
// This test is unable to complete without the system monitor, which is 
// disabled with systematic testing.
#ifdef USE_SYSMONITOR
  harness.run(test1); 
#else
  UNUSED(harness);
#endif
  return 0;
}
