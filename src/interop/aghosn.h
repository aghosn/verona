#pragma once

#include <cassert>
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/Verifier.h"

using namespace std;
using namespace llvm;

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
Value* generate_stupid_call(IRBuilder<>& builder, Module& mod, Function* callee) {
  vector<Value*> args; 
  return builder.CreateCall(callee, args);
}

void generate_sandbox_init(Module& mod) {
  IRBuilder<> builder(mod.getContext());
  vector<Function*> functions;
  //Register all the functions.
  for (auto &f: mod) {
    functions.push_back(&f);
  }
  // Generate stupid function that we will call.
  // TODO this will be replaced with a call to sandboxlib export_function.
  Function* stupid = declare_stupid_function(builder, mod);

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
  for (auto *callee: functions) {
    Value* instr = generate_stupid_call(builder, mod, stupid);
  }
  builder.CreateRet(nullptr);
}
