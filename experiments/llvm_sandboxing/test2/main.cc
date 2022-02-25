#include <process_sandbox/cxxsandbox.h>
#include <process_sandbox/sandbox.h>
int func1(void) {
  return 1;
}
void func2(char c) {return;}
char func3(char a, int b, bool c) {return 'c';}

extern "C" void sandbox_init()
{
  sandbox::ExportedLibrary::export_function(func1);
}

int main(void) {
  //sandbox::ClangExporter::export_function(func1); 
}
