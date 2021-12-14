#include "IRinstr.h"

namespace verona::interop
{
  Value* generate_export_call(
    IRBuilder<>& builder, Module& mod, Function* callee, Function* target)
  {
    vector<Value*> args(1, target);
    return builder.CreateCall(callee, args);
  }

  void init_export_templates(Module& mod)
  {
    static bool inited = false;
    if (inited)
      return;
    for (auto& f : mod)
    {
      std::string name = demangle(f.getName().str());
      if (name.find(exportFunctionName) == 0)
      {
        exportFns.insert(&f);
        FunctionType* tpe = f.getFunctionType();
        assert(tpe != nullptr && tpe->getNumParams() == 1);
        Type* paramtpe = tpe->getParamType(0);
        assert(paramtpe != nullptr);
        tpe2export[paramtpe] = &f;
        cout << "export function " << f.getName().str() << endl;
      }
    }
    // Done with the initilization
    inited = true;
  }

  void generate_sandbox_init(Module& mod)
  {
    IRBuilder<> builder(mod.getContext());
    vector<Function*> functions;
    init_export_templates(mod);

    // Register all the functions.
    for (auto& f : mod)
    {
      if (exportFns.count(&f) == 0)
      {
        functions.push_back(&f);
        cout << "The name of the function " << demangle(f.getName().str())
             << endl;
      }
    }
    // Declare the sandbox_init function
    vector<Type*> voids(0);
    FunctionType* FT =
      FunctionType::get(Type::getVoidTy(mod.getContext()), voids, false);
    Function* proto =
      Function::Create(FT, Function::ExternalLinkage, "sandbox_init", mod);

    // Create its body
    BasicBlock* BB = BasicBlock::Create(mod.getContext(), "entry", proto);
    builder.SetInsertPoint(BB);

    // TODO only do this for functions found in the json config.
    for (auto* callee : functions)
    {
      Type* t = callee->getType();
      if (tpe2export.find(t) != tpe2export.end())
      {
        Function* exporter = tpe2export[t];
        Value* instr = generate_export_call(builder, mod, exporter, callee);
      }
    }
    builder.CreateRet(nullptr);
  }

} // namespace verona::interop;
