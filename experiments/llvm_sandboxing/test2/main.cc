#include <iostream>
int func1(void) {
  return 1;
}
void func2(char c) {
  std::cout << "Hello from func2 " << c << std::endl;
  return;
}
char func3(char a, int b, bool c) {return 'c';}

extern "C" void sandbox_init()
{
}
