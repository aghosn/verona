// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include "ASTinstr.h"
#include "CXXInterface.h"
#include "FS.h"
#include "config.h"

#include <filesystem> // C++17
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace verona::interop;
using namespace clang;
namespace cl = llvm::cl;

/**
 * This file is a helper for a few tests, not the aim of an actual
 * interoperability driver, which will actually be hidden inside the compiler.
 *
 * We should move this into a bunch of unit tests andn run them directly from
 * ctest, with all the functionality we'll need from the sandbox code inside
 * the compiler.
 */

namespace
{
  // Options (@aghosn) for sandboxing instrumentation
  // Turn on sandboxing
  cl::opt<bool> sandbox(
    "sandbox",
    cl::desc("Generate sandbox instrumentation. Expects --targets to be on."),
    cl::Optional,
    cl::init(false));
  // Fully qualified name of the class that exports sandboxed library funcs.
  // Must have a static function called 'export_function'.
  cl::opt<string> sbexporter(
      "sbexporter",
      cl::desc("Fully qualified name of the class registering libray functions"),
      cl::Optional,
      cl::value_desc(sbexporter),
      cl::init("sandbox::ClangExporter"));
  // Supply the list of target functions.
  cl::opt<string> targets(
    "targets",
    cl::desc("<targets file> A list of the library functions to expose"),
    cl::Optional,
    cl::value_desc(targets));

  // For help's sake, will never be parsed, as we intercept
  cl::opt<string> config(
    "config",
    cl::desc("<config file>"),
    cl::Optional,
    cl::value_desc("config"));

  // Test function (TODO: make this more generic)
  cl::opt<bool> testFunction(
    "function",
    cl::desc("Creates a test function"),
    cl::Optional,
    cl::init(false));

  cl::opt<bool> dumpIR(
    "dump",
    cl::desc("Dumps the whole IR at the end"),
    cl::Optional,
    cl::init(false));

  cl::opt<string> inputFile(
    cl::Positional,
    cl::desc("<input file>"),
    cl::init("-"),
    cl::value_desc("filename"));

  cl::opt<string> symbol(
    cl::Positional,
    cl::desc("<symbol>"),
    cl::init(""),
    cl::value_desc("symbol"));

  cl::list<string> specialization(
    "params",
    cl::desc("<template specialization parameters>"),
    cl::CommaSeparated,
    cl::value_desc("specialization"));

  cl::list<string> fields(
    "fields",
    cl::desc("<list of fields to query>"),
    cl::CommaSeparated,
    cl::value_desc("fields"));

  cl::opt<string> method(
    "method",
    cl::desc("<single method to query>"),
    cl::Optional,
    cl::value_desc("method"));

  cl::list<string> argTys(
    "argTys",
    cl::desc("<list of method's argument types to query>"),
    cl::CommaSeparated,
    cl::value_desc("argTys"));

  /// Parse config file adding args to the args globals
  void parseCommandLine(int argc, char** argv, vector<string>& includePath)
  {
    // Parse the command line
    cl::ParseCommandLineOptions(argc, argv, "Verona Interop test\n");
    std::vector<std::string> paths;
    if (!config.empty())
    {
      std::ifstream file(config);
      if (!file.good())
      {
        cerr << "Error opening config file " << config << endl;
        exit(1);
      }
      paths.push_back(config);
    }

    // Parsing targets
    if (!config.empty())
    {
      std::ifstream file(targets);
      if (!file.good())
      {
        cerr << "Error opening targets file " << targets << endl;
        exit(1);
      }

      // Read the file and add all of functions (one per line) to the targets.
      std::string line;
      while (std::getline(file, line))
      {
        target_functions.push_back(line);
      }
    }

    // Adding the exporter class
    exporter_class_name = sbexporter;

    // Add the path to the config files to the include path
    for (auto path : paths)
    {
      auto conf = FSHelper::getRealPath(path);
      auto dir = FSHelper::getDirName(conf);
      if (std::filesystem::is_directory(conf))
      {
        dir = conf;
      }
      includePath.push_back(dir);
    }
  }

  /// Test call
  void test_call(
    CXXType& context,
    clang::CXXMethodDecl* func,
    llvm::ArrayRef<clang::QualType> argTys,
    clang::QualType retTy,
    const CXXInterface& interface)
  {
    const CXXBuilder* builder = interface.getBuilder();

    // Build a unique name (per class/method)
    string fqName = context.getName().str();
    if (context.isTemplate())
    {
      auto params = context.getTemplateParameters();
      if (params)
      {
        for (auto* decl : *params)
        {
          fqName += "_" + decl->getNameAsString();
        }
      }
      fqName += "_";
    }
    fqName += func->getName().str();
    string wrapperName = "__call_to_" + fqName;

    // Create a function with a hygienic name and the same args
    auto caller = builder->buildFunction(wrapperName, argTys, retTy);

    // Collect arguments
    auto args = caller->parameters();

    // Create the call to the actual function
    auto call = builder->createMemberCall(func, args, retTy, caller);

    // Return the call's value
    builder->createReturn(call, caller);
  }

  /// Test a type
  void test_type(
    llvm::StringRef name,
    llvm::ArrayRef<std::string> args,
    llvm::ArrayRef<std::string> fields,
    const CXXInterface& interface)
  {
    const CXXQuery* query = interface.getQuery();
    const CXXBuilder* builder = interface.getBuilder();

    // Find type
    CXXType ty = query->getType(symbol);
    if (!ty.valid())
    {
      cerr << "Invalid type '" << ty.getName().str() << "'" << endl;
      exit(1);
    }

    // Print type name and kind
    cout << "Type '" << ty.getName().str() << "' as " << ty.kindName();
    if (ty.kind == CXXType::Kind::Builtin)
      cout << " (" << ty.builtinKindName() << ")";
    cout << endl;

    // Try and specialize a template
    // TODO: Should this be part of getType()?
    // Do we need a complete type for template parameters?
    if (ty.isTemplate())
    {
      // Tries to instantiate a full specialisation
      ty = builder->buildTemplateType(ty, specialization);
    }

    // If all goes well, this returns a platform-dependent size
    cout << "Size of " << ty.getName().str() << " is " << query->getTypeSize(ty)
         << " bytes" << endl;

    // If requested any field to lookup, by name
    for (auto f : fields)
    {
      auto field = query->getField(ty, f);
      if (!field)
      {
        cerr << "Invalid field '" << f << "' on type '" << ty.getName().str()
             << "'" << endl;
        exit(1);
      }
      auto fieldTy = field->getType();
      auto tyClass = fieldTy->getTypeClassName();
      auto tyName = fieldTy.getAsString();
      cout << "Field '" << field->getName().str() << "' has " << tyClass
           << " type '" << tyName << "'" << endl;
    }

    // If requested any method to lookup, by name and arg types
    if (!method.empty())
    {
      auto func = query->getMethod(ty, method, argTys);
      if (!func)
      {
        cerr << "Invalid method '" << method << "' on type '"
             << ty.getName().str() << "'" << endl;
        exit(1);
      }
      auto fName = func->getName().str();
      cout << "Method '" << fName << "' with signature: (";
      llvm::SmallVector<clang::QualType, 1> argTys;
      for (auto arg : func->parameters())
      {
        if (arg != *func->param_begin())
          cout << ", ";
        auto argTy = arg->getType();
        argTys.push_back(argTy);
        cout << argTy.getAsString() << " " << arg->getName().str();
      }
      auto retTy = func->getReturnType();
      cout << ") -> " << retTy.getAsString() << endl;

      // Instantiate function in AST that calls this method
      test_call(ty, func, argTys, retTy, interface);
    }
  }

  /// Creates a test function
  void test_function(const char* name, const CXXInterface& interface)
  {
    const CXXQuery* query = interface.getQuery();
    const CXXBuilder* builder = interface.getBuilder();

    // Create a new function on the main file
    auto intTy = query->getQualType(CXXType::getInt());
    llvm::SmallVector<clang::QualType, 1> args{intTy};

    // Create new function
    auto func = builder->buildFunction(name, args, intTy);

    // Create constant literal
    auto* fourLiteral = builder->createIntegerLiteral(32, 4);

    // Return statement
    builder->createReturn(fourLiteral, func);
  }
} // namespace

int main(int argc, char** argv)
{
  // Parse cmd-line options
  vector<string> includePath;
  parseCommandLine(argc, argv, includePath);

  // Create the C++ interface
  CXXInterface interface(inputFile, includePath);

  // Test type query
  if (!symbol.empty())
  {
    test_type(symbol, specialization, fields, interface);
  }

  // Test function creation
  if (testFunction)
  {
    test_function("verona_wrapper_fn_1", interface);
  }

  //  Generate the sandbox instrumentation.
  if (sandbox)
  {
    // Check that we have the list of targets.
    specialize_export_function(interface);
  }

  // Dumps the AST before trying to emit LLVM for debugging purposes
  // NOTE: Output is not stable, don't use it for tests
  if (dumpIR)
  {
    interface.dumpAST();
  }

  // Emit whatever is left on the main file
  // This is silent, just to make sure nothing breaks here
  auto mod = interface.emitLLVM();

  // Dump LLVM IR for debugging purposes
  // NOTE: Output is not stable, don't use it for tests
  if (dumpIR)
  {
    mod->dump();
  }

  return 0;
}
