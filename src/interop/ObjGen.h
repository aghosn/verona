#pragma once

#include <llvm/IR/Module.h>

namespace codegen 
{
  
  int generateObjCode(llvm::Module& mod, std::string filename);

} // namespace
