#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

namespace verona::interop
{
 
  // The dispatch function takes an int index and a void* pointer to the argument frame.
  // It contains a switch to dispatch a call to the correct target.
  void generate_dispatch_function(llvm::Module& mod);

} // namespace verona::interop
