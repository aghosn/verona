#include <string>
#include <vector>
#include <llvm/Demangle/Demangle.h>

#include "ASTinstr.h"
#include "IRinstr.h"


namespace verona::interop
{
  static const char* DISPATCHER_NAME = "interop_dispacth_function"; 

  static vector<Function*> find_exporters(Module& mod)
  {
    vector<Function*> result;
    for (auto& f: mod)
    {
      auto fname = demangle(f.getName().str());
      // TODO try to figure out a better way
      if (fname.find(exporter_class_name) != string::npos &&
          fname.find(METHOD_NAME) != string::npos) {
        result.push_back(&f);
        cout << fname << endl;
      }
    }
    return result;
  }

  static Function* find_function(Module& mod, string name)
  {
    for (auto& f: mod)
    {
      auto fname = demangle(f.getName().str());
      auto idx = fname.find('(');
      if (idx != string::npos)
      {
        fname = fname.substr(0, idx);
      }
      if (fname == name) {
        return &f;
      }
    }
    return nullptr;
  }

  void generate_dispatch_function(Module& mod) 
  {
    IRBuilder<> builder(mod.getContext());

    // Find the exporter functions
    auto exporters = find_exporters(mod);
    
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
      auto *f = find_function(mod, target_functions[i]); 
      assert(f != nullptr);
    }

     
    proto->dump();
  }

} // namespace verona::interop
