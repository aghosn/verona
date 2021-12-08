# Sandbox Generating

## Reverse-engineering verona-interop

Without any argument, it defaults to reading from stdin.
Is it a bug?

It's stuck in the interface at `verona::interop::Compiler::ExecuteAction` on GenerateMemoryPCHAction.
Trying to print out config files, and see if this is just opening stdin.
Okay need to look into clang directly.

Okay the default from the API is this, it looks at stdin if there is no inputfile.
`https://clang.llvm.org/doxygen/CompilerInstance_8cpp_source.html#l00940`

They are using llvm::cl::opt to provide options.
See how this works with that [link](#https://llvm.org/docs/CommandLine.html)
Okay so the `-h` works fine, and we can dump the functions. We get the AST and the LLVMIR apparently.
-> I should check the llvm ir format again.

```
# That works
╰─$ ./dist/verona-interop --function --dump /tmp/test.h                                             [14:44]

# That doesn't
╰─$ ./dist/verona-interop --function --dump /tmp/test.h MyStruct                                             [14:44]
```

The verona function is just creating a fake function that takes an int.


```
auto DC = interface.getAstContext()->getTranslationUnitDecl();
    for(auto d: DC->decls()) {
      if (d->isFunctionOrFunctionTemplate()) {
        auto func = d->getAsFunction();
        cout << "One " << func->getNameInfo().getAsString() << endl;
      }
    }

```

find type works with fully qualified names, i.e., you need to specifify the full path (namespace etc.).

Need to write a more generic matcher then...
Because it expects to store a type not a decl.

### At the IR level
```
auto c = mod->getOrInsertFunction("sandbox_init",
      llvm::IntegerType::get(mod->getContext(), 32), NULL);
  auto tmp = (c.getCallee());
  auto sb_init = cast< llvm::Function>(tmp);
  sb_init->setCallingConv(llvm::CallingConv::C);

  llvm::Function::arg_iterator args = sb_init->arg_begin();
  llvm::Value* x = args++;
  x->setName("x");

  llvm::BasicBlock* block = llvm::BasicBlock::Create(mod->getContext(), "entry", sb_init);
  llvm::IRBuilder<> builder(block);
  builder.CreateRet(x);


  // aghosn trying to see if I can get functions here.
  for (auto &F: *mod) {
    llvm::StringRef name = F.getName();
    cout << "A name " << name.str() << endl;
  } 

  // Dump LLVM IR for debugging purposes
  // NOTE: Output is not stable, don't use it for tests
  if (dumpIR)
  {
    mod->dump();
  }

```

```
Function* declare_function(const char* name, Module& mod) {
  std::vector<Type*> voids(0);
  FunctionType *FT = FunctionType::get(
      Type::getVoidTy(mod.getContext()), voids, false);
  Function *F = Function::Create(FT, Function::ExternalLinkage, name, mod);
  return F;

Value* generate_calls(IRBuilder<>& builder, Module& mod) {
  for (auto &f: mod) {
    cout << f.getName().str() << endl;
  }
  return nullptr;
}

void define_function(Function* proto, Module& mod) {
  assert(proto);
  IRBuilder<> builder(mod.getContext());
  BasicBlock *BB = BasicBlock::Create(mod.getContext(), "entry", proto);
  builder.SetInsertPoint(BB);
  generate_calls(builder, mod);
  builder.CreateRet(nullptr);
  verifyFunction(*proto);
}
}
```

### Notes on LLVM:

phi nodes -> when the value comes externally to a basic block.

## TODOs

1. Read documentation about clang and llvm.
