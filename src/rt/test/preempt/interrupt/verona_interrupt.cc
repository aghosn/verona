#include <test/harness.h>
#include <cpp/when.h>
#include <sched/preempt.h>
#include <cassert>

#include <iostream>

using namespace verona::cpp;

#if defined(USE_SYSMONITOR) and defined(USE_PREEMPTION) and not defined(USE_SYSTEMATIC_TESTING)
bool ran = false;

void long_running()
{
  Logging::cout() << "Preempt me if you can" << Logging::endl;
  while(!ran)
  {
    bool preemptable = Preempt::is_preemptable();
    assert(preemptable);
  }
  std::cout << "DONE" << std::endl;
}

void short_running()
{
  Logging::cout() << "Short running behavior" << Logging::endl;
  ran = true;
}

void test_inner()
{
  when() << long_running;
  when() << short_running;
}

void test()
{
  when() << test_inner;
}

#endif

int verona_main(SystematicTestHarness& harness)
{
#if defined(USE_SYSMONITOR) and defined(USE_PREEMPTION) and not defined(USE_SYSTEMATIC_TESTING)
  harness.run(test);
#else
  UNUSED(harness);
#endif
  
  return 0;
}

int main(int argc, char** argv)
{
  SystematicTestHarness harness(argc, argv);

  // This test is meant to run on a single core.
  Logging::cout() << "Running with a single core" << std::endl;
  harness.cores = 1;
  return verona_main(harness);
}
