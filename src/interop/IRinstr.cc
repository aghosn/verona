#include <vector>

#include "IRinstr.h"


namespace verona::interop
{
  static const char* DISPATCHER_NAME = "interop_dispacth_function"; 

  void generate_dispatch_function(Module& mod) 
  {
    IRBuilder<>builder(mod.getContext());
    
    vector<Type*> types {
      Type::getInt64Ty(mod.getContext()),
      Type::getInt8PtrTy(mod.getContext())};
    
    // Declare the function.
    FunctionType* FT = 
      FunctionType::get(Type::getVoidTy(mod.getContext()), types, false);
    Function* proto = 
      Function::Create(FT, Function::ExternalLinkage, DISPATCHER_NAME, mod); 
    
    // Create the function body.
    // It should be a switch.

  }

} // namespace verona::interop
