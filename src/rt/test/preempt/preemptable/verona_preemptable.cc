#include <cpp/when.h>
#include <test/harness.h>
#include <sched/preempt.h>

#include <cassert>

using namespace verona::cpp;

#if defined(USE_SYSMONITOR) and defined(USE_PREEMPTION)

void nested()
{
  assert(Preempt::is_preemptable());
}

void inner()
{
  assert(Preempt::is_preemptable());
  nested();
}

void test()
{
  /// Still outside of a behaviour, preemption has not yet been enabled.
  assert(!Preempt::is_preemptable());
  when() << inner; 
}

#endif

/// Very simple test that checks that user code is preemptable.
int verona_main(SystematicTestHarness& harness)
{
#if defined(USE_SYSMONITOR) and defined(USE_PREEMPTION)
  harness.run(test);
#else
  UNUSED(harness);
#endif
  return 0;
}

int main(int argc, char** argv)
{
  SystematicTestHarness harness(argc, argv);
  return verona_main(harness);
}
