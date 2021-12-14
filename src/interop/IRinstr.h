#pragma once

#include <cassert>
#include <iostream>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <map>
#include <set>

using namespace std;
using namespace llvm;

namespace verona::interop
{
  /**
   * The fully qualified name of the export_function function.
   */
  static const char* exportFunctionName = "void myNameSpace::export_function";
  static map<Type*, Function*> tpe2export;
  static set<Function*> exportFns;

  /**
   * Generates a call to `callee` passing `target` as an argument in the current
   * basic block.
   */
  Value* generate_export_call(
    IRBuilder<>& builder, Module& mod, Function* callee, Function* target);

  /**
   * TODO find the correct way of doing this.
   * And this should be done differently afterwards.
   * And should handle the various templated functions.
   * Which themselves should be generated automatically.
   */
  void init_export_templates(Module& mod);

  /**
   * generate_sandbox_init initializes the export templates and creates the
   * llvmIR for the sandbox_init function with the registered targets.
   */
  void generate_sandbox_init(Module& mod);

} // namespace verona::interop
