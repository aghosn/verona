//#include <process_sandbox/cxxsandbox.h>
#include <iostream>
int foo(int a, int b) {
  return a+b;
}

void bar(int a, int b) {}

void food(){
  std::cout << "From food" << std::endl;
};

extern "C" void sandbox_init()
{}

/*int main(void) {
//  int a = 3;
//  void* ptr = (void*)(&a);
//  sandbox::ClangExporter<void>::call_function(foo, ptr);
  return 0;
}*/
