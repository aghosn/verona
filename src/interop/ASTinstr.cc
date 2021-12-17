#include <iostream>
#include <cassert>
#include <clang/AST/DeclTemplate.h>

#include "ASTinstr.h"

using namespace std;
namespace verona::interop {
  static const char* SANDBOX_INIT = "sandbox_init"; 
  static const char* METHOD_NAME = "export_function";
  static const char* EXPORTER_NAME = "myNameSpace::export_function";
  static const char* EXPORTER_CLASS_NAME = "myNameSpace::ExportedFunction";

  vector<clang::TemplateArgument> build_fn_template_type(clang::FunctionDecl* decl) {
    assert(decl != nullptr);
    vector<clang::TemplateArgument> result;

    // The return type
    result.push_back(decl->getReturnType());

    // The arguments.
    // We have to dynamically allocate for TemplateArgument.
    vector<clang::TemplateArgument>* args = new vector<clang::TemplateArgument>();
    for (auto *p: decl->parameters()) {
      args->push_back(clang::TemplateArgument(p->getType()));
    }

    // Convert the args into a pack Template Argument.
    clang::TemplateArgument packed = clang::TemplateArgument(*args);
    assert(packed.getKind() == clang::TemplateArgument::ArgKind::Pack);

    result.push_back(packed);
    
    // The final result should be (return type, pack(args...))
    return result;
  }

  vector<vector<clang::TemplateArgument>> find_targets_types(const CXXQuery* query) {
    assert(query != nullptr);
    vector<vector<clang::TemplateArgument>> types;
    for (auto target: targets) {
      clang::FunctionDecl *decl = query->getFunction(target); 
      assert(decl != nullptr);
      types.push_back(build_fn_template_type(decl));
    }
    return types;
  }


  /**
   * Exposer is a stupid class that allows us to access the protected
   * method addSpecialization from the FunctionTemplateDecl class.
   * @TODO find a better way to build specialization information.
   */
  class Exposer: public clang::FunctionTemplateDecl {
    public:
      void expose_spec(clang::FunctionTemplateSpecializationInfo* Info, void* InsertPos) {
        this->addSpecialization(Info, InsertPos);
      }
  };

  /**
   * create_info creates the specialization information for a templated
   * function instance.
   */
  clang::FunctionTemplateSpecializationInfo* create_info() {
    // We have the kind
    clang::TemplateSpecializationKind kind = clang::TemplateSpecializationKind::TSK_ImplicitInstantiation;
    return nullptr;
  }

  clang::FunctionDecl* function_specialization(
      CXXInterface& interface,
      clang::FunctionTemplateDecl* base,
      llvm::ArrayRef<clang::TemplateArgument> t)
  {
    assert(base != nullptr);

    // Find previous declaration.
    void* ins_point = nullptr;
    clang::FunctionDecl* retval = base->findSpecialization(t, ins_point);

    // It already exists
    if (retval != nullptr){
      return retval;
    }

    // TODO build the function template specialization.
    return nullptr;
  }

  clang::ClassTemplateSpecializationDecl* class_specialization(CXXInterface& interface,
      clang::ClassTemplateDecl* base,
      llvm::ArrayRef<clang::TemplateArgument> t)
  {
    assert(base != nullptr);

    // Find a previous declaration.
    void *ins_point = nullptr;
    clang::ClassTemplateSpecializationDecl* retval = base->findSpecialization(t, ins_point);

    // It already exists.
    if (retval != nullptr) {
      return retval;
    }

    const CXXBuilder* builder = interface.getBuilder();
    assert(builder != nullptr);
    retval = builder->buildTemplateType(base, t);
    assert(retval != nullptr);

    return retval;
  }

  void specialize_export_function(CXXInterface& interface) {
    const CXXQuery* query = interface.getQuery();
    const CXXBuilder* builder = interface.getBuilder();
    assert(query != nullptr);
    assert(builder != nullptr);
    
    // Find the template export function
    clang::FunctionTemplateDecl* exporter = query->getFunctionTemplate(EXPORTER_NAME);
    assert(exporter != nullptr);
    assert(exporter->isTemplated());

    // Find the ExportedFunction class template.
    clang::ClassTemplateDecl* exporterClass = query->getClassTemplate(EXPORTER_CLASS_NAME);
    assert(exporterClass != nullptr);
    assert(exporterClass->isTemplated());

    // Build the template arguments for the functions we expose.
    vector<vector<clang::TemplateArgument>> types = find_targets_types(query); 


    // Create sandbox_init in the main file
    auto intTy = query->getQualType(CXXType::getInt());
    llvm::SmallVector<clang::QualType, 0> args{};
    auto func = builder->buildFunction(SANDBOX_INIT, args, intTy);
    // TODO Create constant literal for the return, find how to declare void.
    auto* fourLiteral = builder->createIntegerLiteral(32, 4);


    // For all types found, try to specialize both templates.
    for (auto t: types) {
        auto *classSpecialization = class_specialization(interface, exporterClass, t);
        assert(classSpecialization != nullptr);

        clang::CXXMethodDecl* exportFunction = nullptr; 
        for (auto *c: classSpecialization->methods()) {
          if (c->isStatic() && c->getName() == METHOD_NAME) {
            exportFunction = c;
            break;
          }  
        }
        assert(exportFunction != nullptr);

        // TODO Create the call
    }
  }

} // namespace verona::interop;
