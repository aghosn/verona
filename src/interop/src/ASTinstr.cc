#include "ASTinstr.h"

#include <cassert>
#include <clang/AST/DeclTemplate.h>
#include <iostream>
#include <typeinfo>

using namespace std;
namespace verona::interop
{
  static const char* SANDBOX_INIT = "sandbox_init";
  const char* METHOD_NAME = "export_function";
  const char* LIB_DISPATCH = "_sandbox_libraries";

  vector<string> target_functions;
  string exporter_class_name;

  vector<clang::TemplateArgument>
  build_fn_template_type(clang::FunctionDecl* decl)
  {
    assert(decl != nullptr);
    vector<clang::TemplateArgument> result;

    // The return type
    result.push_back(decl->getReturnType());

    // The arguments.
    // We have to dynamically allocate for TemplateArgument.
    vector<clang::TemplateArgument>* args =
      new vector<clang::TemplateArgument>();
    for (auto* p : decl->parameters())
    {
      args->push_back(clang::TemplateArgument(p->getType()));
    }

    // Convert the args into a pack Template Argument.
    clang::TemplateArgument packed = clang::TemplateArgument(*args);
    assert(packed.getKind() == clang::TemplateArgument::ArgKind::Pack);

    result.push_back(packed);

    // The final result should be (return type, pack(args...))
    return result;
  }

  clang::ClassTemplateSpecializationDecl* class_specialization(
    CXXInterface& interface,
    clang::ClassTemplateDecl* base,
    llvm::ArrayRef<clang::TemplateArgument> t)
  {
    assert(base != nullptr);

    // Find a previous declaration.
    void* ins_point = nullptr;
    clang::ClassTemplateSpecializationDecl* retval =
      base->findSpecialization(t, ins_point);

    // It already exists.
    if (retval != nullptr)
    {
      return retval;
    }

    const CXXBuilder* builder = interface.getBuilder();
    assert(builder != nullptr);
    retval = builder->buildTemplateType(base, t);
    assert(retval != nullptr);

    builder->markAsUsed(retval);
    return retval;
  }

  void generate_dispatchers(CXXInterface& interface)
  {
    const CXXQuery* query = interface.getQuery();
    const CXXBuilder* builder = interface.getBuilder();
    assert(query != nullptr);
    assert(builder != nullptr);
    
    // Generate dispatchers for each target
    for (auto target: target_functions)
    {
      clang::FunctionDecl* decl = query->getFunction(target);
      if (decl == nullptr)
      {
        cerr << "Error: could not find function " << target << endl;
      }
      assert(decl != nullptr);
      builder->generateDispatcher(decl);
    }
  }

  void generate_trusted_senders(CXXInterface& interface)
  {
    const CXXQuery* query = interface.getQuery();
    const CXXBuilder* builder = interface.getBuilder();
    assert(query != nullptr);
    assert(builder != nullptr);

    // Find the library dispatcher
    auto dispatcher = query->getVarDecl(LIB_DISPATCH);
    assert(dispatcher != nullptr);

    for (int i = 0; i < target_functions.size(); i++)
    {
      auto target = target_functions[i];
      clang::FunctionDecl* decl = query->getFunction(target);
      if (decl == nullptr)
      {
        cerr << "Error: could not find function " << target << endl;
      }
      assert(decl != nullptr);
      builder->generateTrustedSender(dispatcher, i, decl);
    }
  }

  void specialize_export_function(CXXInterface& interface)
  {
    const CXXQuery* query = interface.getQuery();
    const CXXBuilder* builder = interface.getBuilder();
    assert(query != nullptr);
    assert(builder != nullptr);

    // Find the ExportedFunction class template.
    clang::ClassTemplateDecl* exporterClass =
      query->getClassTemplate(exporter_class_name);
    assert(exporterClass != nullptr);
    assert(exporterClass->isTemplated());

    // Create sandbox_init in the main file
    auto intTy = query->getQualType(CXXType::getInt());
    llvm::SmallVector<clang::QualType, 0> args{};
    auto sbInit = builder->buildFunction(SANDBOX_INIT, args, intTy);
    // TODO Create constant literal for the return, find how to declare void.
    auto* fourLiteral = builder->createIntegerLiteral(32, 42);

    // For all types found, try to specialize the templates.
    // Accumulate calls for the body of sandbox_init.
    std::vector<clang::Stmt*> calls;
    for (auto target : target_functions)
    {
      clang::FunctionDecl* decl = query->getFunction(target);
      assert(decl != nullptr);
      auto t = build_fn_template_type(decl);

      auto* classSpecialization =
        class_specialization(interface, exporterClass, t);
      assert(classSpecialization != nullptr);

      clang::CXXMethodDecl* exportFunction = nullptr;
      for (auto* c : classSpecialization->methods())
      {
        if (exportFunction == nullptr && c->isStatic() && c->getName() == METHOD_NAME)
        {
          exportFunction = c;
          break;
        } 
      }
      assert(exportFunction != nullptr);
      std::vector<clang::ValueDecl*> args;
      args.push_back(decl);
      auto call = builder->createMemberCallFunctionArg(
        exportFunction, args, intTy, sbInit);
      calls.push_back(call);
    }
    clang::SourceLocation loc = sbInit->getLocation();
    // Return statement
    auto retStmt = clang::ReturnStmt::Create(
      *builder->getASTContext(), sbInit->getLocation(), fourLiteral, nullptr);
    calls.push_back(retStmt);
    auto compStmt =
      clang::CompoundStmt::Create(*builder->getASTContext(), calls, loc, loc);
    sbInit->setBody(compStmt);
    builder->markAsUsed(sbInit);
  }
} // namespace verona::interop;
