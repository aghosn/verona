//#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetRegistry.h>



#include "ObjGen.h"

using namespace llvm;
using namespace llvm::sys;

namespace codegen
{
  
  int generateObjCode(llvm::Module& mod, std::string filename)
  {
    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
    auto targetTriple = sys::getDefaultTargetTriple();
    mod.setTargetTriple(targetTriple);

    std::string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);
    if (!target)
    {
      errs() << error;
      return 1;
    }
    
    auto cpu = "generic";
    auto features = "";
    TargetOptions opt;
    auto rm = Optional<Reloc::Model>(); 
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, 
        features, opt, rm);
    mod.setDataLayout(targetMachine->createDataLayout());
    
    std::error_code ec;
    raw_fd_ostream dest(filename, ec, sys::fs::OF_None);
    if (ec)
    {
      errs() << "Could not open file: " << ec.message();
      return 1;
    }

    legacy::PassManager pass;
    auto fileType = CGFT_ObjectFile;

    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType))
    {
      errs() << "Target machine cannot emit a file of this type ";
      return 1;
    }
    pass.run(mod);
    dest.flush();
    return 0;
  }

} // namespace
