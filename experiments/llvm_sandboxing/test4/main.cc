struct Library
{
  void send(int idx, void* ptr){}
};

Library* _sandbox_libraries;

extern "C" int foo(int a, int b);


extern "C" void food();

int bar()
{
  return 3;
}

int food(int c, int d) {
  struct Argument
  {
    int a;
    int b;
    int ret;
  };
  Argument zoob;
  zoob.a = c;
  zoob.b = d;
  _sandbox_libraries->send(0, (void*)&zoob); 
  return zoob.ret;
}
