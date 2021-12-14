#include <iostream>
#include <cassert>
#include <clang/AST/DeclTemplate.h>

#include "ASTinstr.h"

using namespace std;
namespace verona::interop {
  
  static const char* EXPORTER_NAME = "myNameSpace::export_function";

  vector<clang::QualType> find_targets_types(const CXXQuery* query) {
    assert(query != nullptr);
    vector<clang::QualType> types;
    for (auto target: targets) {
      clang::FunctionDecl *decl = query->getFunction(target); 
      assert(decl != nullptr);
      types.push_back(decl->getType());
    }
    return types;
  }

  vector<vector<clang::TemplateArgument>> find_targets_types2(const CXXQuery* query) {
    assert(query != nullptr);
    vector<vector<clang::TemplateArgument>> types;
    for (auto target: targets) {
      clang::FunctionDecl *decl = query->getFunction(target); 
      assert(decl != nullptr);
      vector<clang::TemplateArgument> args;
      args.push_back(clang::TemplateArgument(decl->getReturnType()));
      for (auto *p: decl->parameters()) {
        args.push_back(clang::TemplateArgument(p->getType()));
      }
      types.push_back(args);
    }
    return types;
  }

  class Exposer: public clang::FunctionTemplateDecl {
    public:
      void expose_spec(clang::FunctionTemplateSpecializationInfo* Info, void* InsertPos) {
        this->addSpecialization(Info, InsertPos);
      }
  };

  void specialize_export_function(const CXXQuery* query) {
    // Find the export function
    assert(query != nullptr);
    clang::FunctionTemplateDecl* exporter = query->getFunctionTemplate(EXPORTER_NAME);
    assert(exporter != nullptr);
    assert(exporter->isTemplated());

    // Now find the types
    vector<vector<clang::TemplateArgument>> types = find_targets_types2(query); 

    // For all types found, try to specialize the template.
    for (auto t: types) {
      void* ins_point = nullptr;
      clang::FunctionDecl* retval = exporter->findSpecialization(t, ins_point);
      
      // It already exists
      if (retval != nullptr) {
        continue;
      }

      //TODO remove afterwards this is bullshit I test to debug.
      clang::FunctionDecl* instantiated = query->getFunction(EXPORTER_NAME);
      assert(instantiated != nullptr);

      cout << "Oh fuck " << instantiated->getType().getAsString() << endl;
      cout << "Fuck: " << instantiated->isFunctionTemplateSpecialization() << endl;
      Exposer* exposer = static_cast<Exposer*>(exporter);
      clang::FunctionTemplateSpecializationInfo* inf = instantiated->getTemplateSpecializationInfo();
      assert(inf != nullptr);
      cout << "The TK: " << inf->getTemplateSpecializationKind() << endl;
      exposer->expose_spec(instantiated->getTemplateSpecializationInfo(), ins_point);
      
      // We now try to instantiate it.
      //assert(ins_point == nullptr);
    }
  }

} // namespace verona::interop;
