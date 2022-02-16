#include <cassert>
#include "test.h"

int func1(void) {return 1;}
void func2(char c) {return;}
char func3(char a, int b, bool c) {return 'c';}


int main(void) {
  myNameSpace::ExportedFunction<int>::export_function(func1);
  //myNameSpace::export_function(func1);
  //myNameSpace::export_function(func2);
  //myNameSpace::export_function(func3);
  //assert(myNameSpace::functions[0] != nullptr);
  //assert(myNameSpace::functions[1] == nullptr);
  return 0;
}
