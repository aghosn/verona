#include <vector>
#include <llvm/Demangle/Demangle.h>

#include "ASTinstr.h"
#include "IRinstr.h"


namespace verona::interop
{
  static const char* DISPATCHER_NAME = "interop_dispacth_function"; 

  static Function* find_exporters(Module& mod)
  {
    vector<Function*> result;
    auto name = exporter_class_name + METHOD_NAME;

    for (auto& f: mod)
    {
      cout << "The name of the function: " << demangle(f.getName().str()) << endl;
    }
    return nullptr;
  }

  void generate_dispatch_function(Module& mod) 
  {
    IRBuilder<> builder(mod.getContext());
    
    vector<Type*> types {
      Type::getInt64Ty(mod.getContext()),
      Type::getInt8PtrTy(mod.getContext())};
    
    // Declare the function.
    FunctionType* FT = 
      FunctionType::get(Type::getVoidTy(mod.getContext()), types, false);
    Function* proto = 
      Function::Create(FT, Function::ExternalLinkage, DISPATCHER_NAME, mod); 
    
    // Create the function body: a switch on the first argument.
    auto index = proto->getArg(0);
    
    // Default case.
    BasicBlock *body = BasicBlock::Create(mod.getContext(), "body", proto);
    builder.SetInsertPoint(body);
    
    SwitchInst *SI = builder.CreateSwitch(index, body, target_functions.size());
    for (int i = 0; i < target_functions.size(); i++) 
    {
      BasicBlock *BC = BasicBlock::Create(mod.getContext(), "C", proto);
      SI->addCase(ConstantInt::get(mod.getContext(), APInt(64, i, true)), BC);
    }

    find_exporters(mod);
    cout << "------------\n\n" << endl;
    
    proto->dump();
  }

} // namespace verona::interop
