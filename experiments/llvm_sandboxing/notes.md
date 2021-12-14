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

```

### Notes on LLVM:

phi nodes -> when the value comes externally to a basic block.

## TODOs

1. Read documentation about clang and llvm.

### Implementation plan

1. See if I can get the sandbox library linked.
2. Find it from the module.
3. Use it as a target instead of stupid_function.

OR

1. Take the one I define from the agtests
2. Register calls.
3. Then try to replace it with the one from the library.

The second option might be better.

```

in __GI___assert_fail (assertion=0x555558401d42 "(i >= FTy->getNumParams() || FTy->getParamType(i) == Args[i]->getType()) && \"Calling a function with a bad signature!\"", file=0x555558401591 "/agent/_work/1/s/llvm/lib/IR/Instructions.cpp", line=498, function=0x555558401c28 "void llvm::CallInst::init(llvm::FunctionType *, llvm::Value *, ArrayRef<llvm::Value *>, ArrayRef<llvm::OperandBundleDef>, const llvm::Twine &)") at assert.c:101

```


### Status right now

Problem is that I cannot instantiate a specialization apparently.
Second problem is the template argument format, the args from the function are ArgPack kind.
-> I can try to provide that sort of functionality, at least it might be useful in the future.

We gonna have the same issue afterwards I guess with the ExportedFunction class that we need to instantiate.
That's a bummer...
 Apprently Renato does not know either.

The other thing we can do now is to try to instantiate the class specialization for the ExportedFunction class. 


Saving some bullshit code here:

```

//TODO remove afterwards this is bullshit I test to debug.
      clang::FunctionDecl* instantiated = query->getFunction(EXPORTER_NAME);
      assert(instantiated != nullptr);

      cout << "Instantiated signature " << instantiated->getType().getAsString() << endl;
      assert(instantiated->isFunctionTemplateSpecialization());
      Exposer* exposer = static_cast<Exposer*>(exporter);
      clang::FunctionTemplateSpecializationInfo* inf = instantiated->getTemplateSpecializationInfo();
      assert(inf != nullptr);
      assert(inf->getTemplateSpecializationKind() == clang::TemplateSpecializationKind::TSK_ImplicitInstantiation);
      auto argArray = inf->TemplateArguments->asArray();
      for (auto a: argArray) {
        if (a.getKind() == clang::TemplateArgument::ArgKind::Type) {
          cout << "An argument " << a.getAsType().getAsString() << endl;
        } else if (a.getKind() == clang::TemplateArgument::ArgKind::Pack) {
          auto unpacked = a.getPackAsArray();
          for (auto p: unpacked) {
            assert(p.getKind() == clang::TemplateArgument::ArgKind::Type);
            cout << "The packed argument " << p.getAsType().getAsString() << endl;
          }
        }
      }
      // TODO this line fails obviously because the specialization already exists.
      //exposer->expose_spec(instantiated->getTemplateSpecializationInfo(), ins_point);
      
      // We now try to instantiate it.
      //assert(ins_point == nullptr);

```
