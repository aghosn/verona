#pragma once

#include <map>
#include <set>
#include <cassert>
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Demangle/Demangle.h>

using namespace std;
using namespace llvm;

/**
 * The fully qualified name of the export_function function.
 */
static const char* exportFunctionName = "void myNameSpace::export_function";
map<FunctionType*, Function*> tpe2export;
set<Function*> exportFns; 


Function* declare_stupid_function(IRBuilder<>& builder, Module& mod) {
  // Declare the function
  vector<Type*> voids(0);
  FunctionType *FT = FunctionType::get(Type::getVoidTy(mod.getContext()), voids, false);
  Function *proto = Function::Create(FT, Function::ExternalLinkage, "stupid_function", mod);

  // Fill its body
  BasicBlock *BB = BasicBlock::Create(mod.getContext(), "entry", proto);
  builder.SetInsertPoint(BB);
  builder.CreateRet(nullptr);
  verifyFunction(*proto);

  return proto;
}

//TODO this is just stupid
Value* generate_export_call(IRBuilder<>& builder, Module& mod, Function* callee, Function* target) {
  vector<Value*> args(1, target); 
  return builder.CreateCall(callee, args);
}

/**
 * TODO find the correct way of doing this.
 * And this should be done differently afterwards.
 * And should handle the various templated functions.
 * Which themselves should be generated automatically.
 */
static set<Function*> findExportFunction(Module& mod) {
  set<Function*> templates;
  for (auto &f: mod) {
    std::string name = demangle(f.getName().str());
    if (name.find(exportFunctionName) == 0) {
      cout << "inserting " << f.getName().str() << endl;
      templates.insert(&f);
    }
  }
  return templates;
}

static void init_export_templates(Module& mod) {
  static bool inited = false;
  if (inited) 
    return;
  for(auto &f: mod) {
    std::string name = demangle(f.getName().str());
    if (name.find(exportFunctionName) == 0) {
      exportFns.insert(&f);
      tpe2export[f.getFunctionType()] = &f;
    }
  }
  // Done with the initilization
  inited = true;
}

static bool type_match(Function* callee, Value* arg) {
  FunctionType* tpe = callee->getFunctionType();
  assert(tpe->getNumParams() == 1); 
  auto *argType = tpe->getParamType(0);
  return (argType == arg->getType());
}

void generate_sandbox_init(Module& mod) {
  IRBuilder<> builder(mod.getContext());
  vector<Function*> functions;
  set<Function*> templates = findExportFunction(mod);

  //Register all the functions.
  for (auto &f: mod) {
    if (templates.count(&f) == 0) {
      functions.push_back(&f);
      cout << "The name of the function " << demangle(f.getName().str()) << endl; 
    }
  }
  // Declare the sandbox_init function
  vector<Type*> voids(0);
  FunctionType *FT = FunctionType::get(Type::getVoidTy(mod.getContext()), voids, false);
  Function *proto = Function::Create(FT, Function::ExternalLinkage, "sandbox_init", mod);

  // Create its body
  BasicBlock *BB = BasicBlock::Create(mod.getContext(), "entry", proto);
  builder.SetInsertPoint(BB);

  // For each of the functions listed, create a call to target function.
  // TODO for the moment we only call the stupid function, next we'll call
  // the export function.
  auto exporter = *(templates.begin());
  for (auto *callee: functions) {
    if (type_match(exporter, callee)) {
      Value* instr = generate_export_call(builder, mod, exporter, callee);
    }
  }
  builder.CreateRet(nullptr);
}
