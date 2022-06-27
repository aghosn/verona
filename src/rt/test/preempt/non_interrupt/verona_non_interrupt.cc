#include <test/harness.h>
#include <cpp/when.h>
#include <sched/preempt.h>


void long_running()
{
  Logging::cout() << "Preempt me if you can (but shouldn't)" << Logging::endl;
  {
    NO_PREEMPT();
    size_t nb_interrupt = verona::rt::Preempt::get_interrupts();
    size_t nb_preempt = verona::rt::Preempt::get_preemptions();
    while(nb_interrupt == verona::rt::Preempt::get_interrupts())
    {
      Logging::cout() << "In the loop." << Logging::endl;
      yield();
    }
    if(nb_preempt != verona::rt::Preempt::get_preemptions())
    {
      Logging::cout() << "This behavior should not have been preempted!" << Logging::endl;
      abort();
    }
  }
  Logging::cout() << "Preempt me if you can (and you should)" << Logging::endl;
  {
    size_t nb_preempt = verona::rt::Preempt::get_preemptions();
    while(nb_preempt == verona::rt::Preempt::get_preemptions())
    {
      Logging::cout() << "In the second loop." << Logging::endl;
      yield();
    }
  }
}

void short_running()
{
  Logging::cout() << "Short running behavior" << Logging::endl;
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

int verona_main(SystematicTestHarness& harness)
{
#ifdef USE_PREEMPTION
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
