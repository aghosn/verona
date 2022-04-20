#include <iostream>
#include <sched2/runtime.h>

#include "sched2.h"

int main(int argc, char** argv)
{
  UNUSED(argc);
  UNUSED(argv);
  verona::rt::Runtime& rt = verona::rt::Runtime::get();
  rt.init(4);
  std::cout << "Hello World" << std::endl;
  return 0;
}
