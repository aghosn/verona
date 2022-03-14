struct Library
{
  void send(int idx, void* ptr){}
};

Library* lib;

extern "C" int foo(int a, int b);


extern "C" void food();

int bar()
{
  return 3;
}

int food(int a, int b) {
  struct Argument
  {
    int a;
    int b;
    int ret;
  };
  Argument zoob;
  zoob.a = a;
  zoob.b = b;
  lib->send(0, (void*)&zoob); 
  return zoob.ret;
}
