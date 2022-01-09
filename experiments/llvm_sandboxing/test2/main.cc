#include <process_sandbox/cxxsandbox.h>
int func1(void) {return 1;}
void func2(char c) {return;}
char func3(char a, int b, bool c) {return 'c';}


int main(void) {
  sandbox::ClangExporter::export_function(func1); 
}
