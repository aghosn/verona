#pragma once
#include <memory>

namespace myNameSpace {
  class MyClass {
    public:
      int myAttribute1;
      char myAttribute2;
      void myMethod(int a);
  };
  
  struct MyStruct {
    int a;
    int b;
  };
  
  
  int myFunc1(int a) {return 3;};
  int myFunc2(int b) {
    return myFunc1(b) + 3333;
  }

  struct ExportedFunctionBase {};

  static ExportedFunctionBase* functions[100];

  template<typename Ret, typename... Args>
  class ExportedFunction: public ExportedFunctionBase {
    public:
      Ret (*function) (Args...);
    ExportedFunction(Ret(*fn)(Args...)) : function(fn) {}

    static int export_function(Ret(*fn)(Args...)) {
      functions[0] = new ExportedFunction<Ret, Args...>(fn);
      return 1;
    }
  };


  template<typename Ret, typename... Args >
  static void export_function(Ret (*fn)(Args...)) {
    functions[0] = new ExportedFunction<Ret, Args...>(fn);
  }

} // namespace myNameSpace;
