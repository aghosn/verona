#pragma once

#include <string>
#include <llvm/IR/Module.h>

namespace codegen 
{
  
  int generateObjCode(llvm::Module& mod, std::string filename);
  void linkObject(std::string objname, std::string target);

} // namespace
