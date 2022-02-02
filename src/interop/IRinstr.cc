#include <string>
#include <vector>
#include <llvm/Demangle/Demangle.h>

#include "ASTinstr.h"
#include "IRinstr.h"


namespace verona::interop
{
  static const char* DISPATCHER_NAME = "interop_dispacth_function"; 
  static const char* DISPATCH_METHOD_NAME = "call_function";

  static vector<Function*> find_exporters(Module& mod)
  {
    vector<Function*> result;
    for (auto& f: mod)
    {
      auto fname = demangle(f.getName().str());
      // TODO try to figure out a better way
      if (fname.find(exporter_class_name) != string::npos &&
          fname.find(DISPATCH_METHOD_NAME) != string::npos) {
        result.push_back(&f);
      }
    }
    return result;
  }

  static Function* find_function(Module& mod, string name)
  {
    auto dispatcher = DISP_PREFIX + name; 
    for (auto& f: mod)
    {
      auto fname = demangle(f.getName().str());
      if (fname.find(dispatcher) != std::string::npos)
      {
        return &f;
      }
    }
    /*for (auto& f: mod)
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
    }*/
    return nullptr;
  }

  static Function* find_exporter(vector<Function*> exporters, Function* target)
  {
    assert(target != nullptr);
    for (auto f: exporters)
    {
      assert(f->arg_size() == 1);
      auto arg = f->getArg(0);
      if (arg->getType() == target->getType()) {
        return f;
      }
    }
    return nullptr;
  }

  void generate_dispatch_function(Module& mod) 
  {
    IRBuilder<> builder(mod.getContext());
    vector<Type*> types {
      Type::getInt64Ty(mod.getContext()),
      Type::getInt8PtrTy(mod.getContext())};
    auto exporters = find_exporters(mod);
    
    // Declare the function.
    FunctionType* FT = 
      FunctionType::get(Type::getVoidTy(mod.getContext()), types, false);
    Function* proto = 
      Function::Create(FT, Function::ExternalLinkage, DISPATCHER_NAME, mod); 
    
    // Create the function body: a switch on the first argument.
    auto index = proto->getArg(0);
    
    // Body.
    BasicBlock *body = BasicBlock::Create(mod.getContext(), "body", proto);
    builder.SetInsertPoint(body);

    // End.
    BasicBlock *end = BasicBlock::Create(mod.getContext(), "end", proto);

    SwitchInst *SI = builder.CreateSwitch(index, body, target_functions.size());
    for (int i = 0; i < target_functions.size(); i++) 
    {
      BasicBlock *BC = BasicBlock::Create(mod.getContext(), "C", proto);
      SI->addCase(ConstantInt::get(mod.getContext(), APInt(64, i, true)), BC);
      auto *f = find_function(mod, target_functions[i]); 
      assert(f != nullptr);
      vector<Value*> args {proto->getArg(1)};
      builder.SetInsertPoint(BC);
      auto call = builder.CreateCall(f, args);
      BranchInst* BI = BranchInst::Create(end);
      BI->insertAfter(call);
    }
    builder.SetInsertPoint(end);
    builder.CreateRet(nullptr);
  }

} // namespace verona::interop
