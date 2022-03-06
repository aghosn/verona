#include <iostream>

void foo()
{
  std::cout << "Hello from foo in the sandbox" << std::endl;
}

extern "C" void sandbox_init() 
{}
