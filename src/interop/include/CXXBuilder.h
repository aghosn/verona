// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "CXXQuery.h"
#include "CXXType.h"
#include "Compiler.h"

#include <iostream>
#include <string>
#include <cassert>

namespace verona::interop
{

  static const char* DISP_PREFIX = "_verona_proxy_";
  /**
   * C++ Builder Interface.
   *
   * This is a class that builds Clang AST expressions. There are a number of
   * helpers to add expressions, calls, types, functions, etc.
   */
  class CXXBuilder
  {
    /// The AST root
    clang::ASTContext* ast = nullptr;
    /// Compiler
    const Compiler* Clang;
    /// Query system
    const CXXQuery* query;

    /**
     * Instantiate the class template specialisation at the end of the main
     * file, if not yet done.
     */
    CXXType instantiateClassTemplate(
      CXXType& classTemplate,
      llvm::ArrayRef<clang::TemplateArgument> args) const
    {
      if (classTemplate.kind != CXXType::Kind::TemplateClass)
      {
        return CXXType{};
      }

      // Check if this specialisation is already present in the AST
      // (declaration, definition, used).
      clang::ClassTemplateDecl* ClassTemplate =
        classTemplate.getAs<clang::ClassTemplateDecl>();
      void* InsertPos = nullptr;
      clang::ClassTemplateSpecializationDecl* Def =
        _instantiateClassTemplate(ClassTemplate, args);
      return CXXType{Def};
    }

    clang::ClassTemplateSpecializationDecl* _instantiateClassTemplate(
      clang::ClassTemplateDecl* ClassTemplate,
      llvm::ArrayRef<clang::TemplateArgument> args) const
    {
      auto& S = Clang->getSema();
      void* InsertPos = nullptr;
      clang::ClassTemplateSpecializationDecl* Decl =
        ClassTemplate->findSpecialization(args, InsertPos);
      if (!Decl)
      {
        // This is the first time we have referenced this class template
        // specialization. Create the canonical declaration and add it to
        // the set of specializations.
        Decl = clang::ClassTemplateSpecializationDecl::Create(
          *ast,
          ClassTemplate->getTemplatedDecl()->getTagKind(),
          ClassTemplate->getDeclContext(),
          ClassTemplate->getTemplatedDecl()->getBeginLoc(),
          ClassTemplate->getLocation(),
          ClassTemplate,
          args,
          nullptr);
        ClassTemplate->AddSpecialization(Decl, InsertPos);
      }
      // If specialisation hasn't been directly declared yet (by the user),
      // instantiate the declaration.
      if (Decl->getSpecializationKind() == clang::TSK_Undeclared)
      {
        clang::MultiLevelTemplateArgumentList TemplateArgLists;
        TemplateArgLists.addOuterTemplateArguments(args);
        S.InstantiateAttrsForDecl(
          TemplateArgLists, ClassTemplate->getTemplatedDecl(), Decl);
      }
      // If specialisation hasn't been defined yet, create its definition at the
      // end of the file.
      clang::ClassTemplateSpecializationDecl* Def =
        clang::cast_or_null<clang::ClassTemplateSpecializationDecl>(
          Decl->getDefinition());
      if (!Def)
      {
        clang::SourceLocation InstantiationLoc = Clang->getEndOfFileLocation();
        assert(InstantiationLoc.isValid());
        S.InstantiateClassTemplateSpecialization(
          InstantiationLoc, Decl, clang::TSK_ExplicitInstantiationDefinition);
        S.InstantiateClassTemplateSpecializationMembers(
          InstantiationLoc, Decl, clang::TSK_ExplicitInstantiationDefinition);
        Def = clang::cast<clang::ClassTemplateSpecializationDecl>(
          Decl->getDefinition());
      }

      // TODO I modified the following lines because they faulted.
      // The lexical context from def was different than DC.
      // This seems to fix it.
      auto* DC = ast->getTranslationUnitDecl();
      Def->setLexicalDeclContext(DC);
      DC->addDecl(Def);
      return Def;
    }

    /**
     * Get a CXXMethod expression as a function pointer
     */
    clang::Expr* getCXXMethodPtr(
      clang::CXXMethodDecl* method, clang::SourceLocation loc) const
    {
      clang::DeclarationNameInfo info;
      clang::NestedNameSpecifierLoc spec;

      // TODO: Implement dynamic calls (MemberExpr)
      assert(method->isStatic());

      // Create a method reference expression
      auto funcTy = method->getFunctionType();
      auto funcQualTy = clang::QualType(funcTy, 0);
      auto expr = clang::DeclRefExpr::Create(
        *ast,
        spec,
        loc,
        method,
        /*capture=*/false,
        info,
        funcQualTy,
        clang::VK_LValue);

      // Implicit cast to function pointer
      auto implCast = clang::ImplicitCastExpr::Create(
        *ast,
        ast->getPointerType(funcQualTy),
        clang::CK_FunctionToPointerDecay,
        expr,
        /*base path=*/nullptr,
        clang::VK_PRValue,
        clang::FPOptionsOverride());

      return implCast;
    }

    /**
     * Call a CXXMethod (pointer or member)
     */
    clang::Expr* callCXXMethod(
      clang::CXXMethodDecl* method,
      clang::Expr* expr,
      llvm::ArrayRef<clang::Expr*> args,
      clang::QualType retTy,
      clang::SourceLocation loc) const
    {
      if (method->isStatic())
      {
        // Static call to a member pointer
        assert(llvm::dyn_cast<clang::ImplicitCastExpr>(expr));

        return clang::CallExpr::Create(
          *ast,
          expr,
          args,
          retTy,
          clang::VK_PRValue,
          loc,
          clang::FPOptionsOverride());
      }
      else
      {
        // Dynamic call to MemberExpr
        assert(llvm::dyn_cast<clang::MemberExpr>(expr));

        return clang::CXXMemberCallExpr::Create(
          *ast,
          expr,
          args,
          retTy,
          clang::VK_PRValue,
          loc,
          clang::FPOptionsOverride());
      }
    }

    /**
     * Return the actual type from a template specialisation if the type
     * is a substitution template type, otherwise just return the type itself.
     */
    clang::QualType getSpecialisedType(clang::QualType ty) const
    {
      if (auto tmplTy = ty->getAs<clang::SubstTemplateTypeParmType>())
        return tmplTy->getReplacementType();
      return ty;
    }

  public:
    /**
     * CXXInterface c-tor. Creates the internal compile unit, include the
     * user file (and all dependencies), generates the pre-compiled headers,
     * creates the compiler instance and re-attaches the AST to the interface.
     */
    CXXBuilder(clang::ASTContext* ast, Compiler* Clang, const CXXQuery* query)
    : ast(ast), Clang(Clang), query(query)
    {}

    /**
     * Build a template class from a CXXType template and a list of type
     * parameters by name.
     *
     * FIXME: Recursively scan the params for template parameters and define
     * them too.
     */
    CXXType
    buildTemplateType(CXXType& ty, llvm::ArrayRef<std::string> params) const
    {
      // Gather all arguments, passed and default
      auto args = query->gatherTemplateArguments(ty, params);

      // Build the canonical representation
      query->getCanonicalTemplateSpecializationType(ty.decl, args);

      // Instantiate and return the definition
      return instantiateClassTemplate(ty, args);
    }

    clang::ClassTemplateSpecializationDecl* buildTemplateType(
      clang::ClassTemplateDecl* ty,
      llvm::ArrayRef<clang::TemplateArgument> params) const
    {
      return _instantiateClassTemplate(ty, params);
    }

    /**
     * Build a function from name, args and return type, if the function
     * does not yet exist. Return the existing one if it does.
     */
    clang::FunctionDecl* buildFunction(
      llvm::StringRef name,
      llvm::ArrayRef<clang::QualType> args,
      clang::QualType retTy) const
    {
      auto* DC = ast->getTranslationUnitDecl();
      clang::SourceLocation loc = Clang->getEndOfFileLocation();
      clang::IdentifierInfo& fnNameIdent = ast->Idents.get(name);
      clang::DeclarationName fnName{&fnNameIdent};
      clang::FunctionProtoType::ExtProtoInfo EPI;

      // Simplify argument types if template specialisation
      llvm::SmallVector<clang::QualType> argTys;
      for (auto argTy : args)
      {
        argTys.push_back(getSpecialisedType(argTy));
      }
      retTy = getSpecialisedType(retTy);

      // Get function type of args/ret
      clang::QualType fnTy = ast->getFunctionType(retTy, argTys, EPI);

      // Create a new function
      auto func = clang::FunctionDecl::Create(
        *ast,
        DC,
        loc,
        loc,
        fnName,
        fnTy,
        ast->getTrivialTypeSourceInfo(fnTy),
        clang::StorageClass::SC_None);

      // Define all arguments
      llvm::SmallVector<clang::ParmVarDecl*, 4> argDecls;
      size_t argID = 0;
      for (auto argTy : argTys)
      {
        std::string argName = "arg" + std::to_string(argID++);
        clang::IdentifierInfo& ident = ast->Idents.get(argName);
        clang::ParmVarDecl* arg = clang::ParmVarDecl::Create(
          *ast,
          func,
          loc,
          loc,
          &ident,
          argTy,
          nullptr,
          clang::StorageClass::SC_None,
          nullptr);
        argDecls.push_back(arg);
      }

      // Set function argument list
      func->setParams(argDecls);

      // Associate with the translation unit
      func->setLexicalDeclContext(DC);
      DC->addDecl(func);

      return func;
    }

    /**
     * Create integer constant literal
     *
     * TODO: Create all the ones we use in the template specialisation
     */
    clang::IntegerLiteral*
    createIntegerLiteral(unsigned int len, unsigned long val) const
    {
      llvm::APInt num{len, val};
      auto* lit = clang::IntegerLiteral::Create(
        *ast,
        num,
        query->getQualType(CXXType::getInt()),
        clang::SourceLocation{});
      return lit;
    }

    /**
     * Create a call instruction
     *
     * TODO: Create all the ones we use in code generation
     */
    clang::Expr* createMemberCall(
      clang::CXXMethodDecl* method,
      llvm::ArrayRef<clang::ParmVarDecl*> args,
      clang::QualType retTy,
      clang::FunctionDecl* caller) const
    {
      clang::SourceLocation loc = caller->getLocation();
      clang::DeclarationNameInfo info;
      clang::NestedNameSpecifierLoc spec;
      auto& S = Clang->getSema();

      // Create an expression for each argument
      llvm::SmallVector<clang::Expr*, 1> argExpr;
      for (auto arg : args)
      {
        // Get expression from declaration
        auto e = clang::DeclRefExpr::Create(
          *ast,
          spec,
          loc,
          arg,
          /*capture=*/false,
          info,
          arg->getType(),
          clang::VK_LValue);

        // Implicit cast to r-value
        auto cast = clang::ImplicitCastExpr::Create(
          *ast,
          e->getType(),
          clang::CK_LValueToRValue,
          e,
          /*base path=*/nullptr,
          clang::VK_PRValue,
          clang::FPOptionsOverride());

        argExpr.push_back(cast);
      }

      // Create a call to the method
      auto expr = getCXXMethodPtr(method, loc);
      auto callStmt = callCXXMethod(method, expr, argExpr, retTy, loc);
      auto compStmt = clang::CompoundStmt::Create(*ast, {callStmt}, loc, loc);

      // Mark method as used
      clang::AttributeCommonInfo CommonInfo = {clang::SourceRange{}};
      method->addAttr(clang::UsedAttr::Create(S.Context, CommonInfo));

      // Update the body and return
      caller->setBody(compStmt);
      return callStmt;
    }

    clang::RecordDecl* generateArgStruct(std::vector<clang::QualType> types, 
        clang::SourceLocation loc,
        std::string sname,
        bool hasReturn) const
    {
      //clang::IdentifierInfo& structName = ast->Idents.get("tmp_struct");
      auto record = ast->buildImplicitRecord(sname);
      record->startDefinition();
      //std::vector<clang::FieldDecl*> fields;
      int counter = 0;
      for (auto type: types)
      {
        auto name = "a"+std::to_string(counter);
        if (hasReturn && counter == types.size() -1)
        {
          name = "ret";
        }
        clang::IdentifierInfo& fieldName = ast->Idents.get(name);
        auto field = clang::FieldDecl::Create(*ast, record, loc, loc, 
            &fieldName, type, ast->getTrivialTypeSourceInfo(type), nullptr, 
            true, clang::ICIS_NoInit); 
        field->setAccess(clang::AS_public);
        record->addDecl(field);
        //fields.push_back(field);
        counter++;
      }
      record->completeDefinition();
      return record;
    }

    void generateDispatcher(clang::FunctionDecl* target) const
    {
      clang::QualType voidQual = ast->VoidTy; 
      auto voidStar = ast->getPointerType(voidQual);
      llvm::SmallVector<clang::QualType, 0> args{voidStar};
      std::string name = DISP_PREFIX + target->getName().str();

      // All proxies have `void` return type.
      auto proxy = buildFunction(name, args, voidQual);
      
      // Generate a struct that corresponds to target arguments.
      auto loc = proxy->getLocation(); 
      std::vector<clang::QualType> types;
      for (auto p: target->parameters())
      {
        if (p->getType() == voidQual)
          continue;
        types.push_back(p->getType());
      }

      // Handle return type.
      bool hasReturn = false;
      if (target->getReturnType() != voidQual) {
        hasReturn = true;
        types.push_back(target->getReturnType());
      } 
      auto record = generateArgStruct(types, loc, "tmp_struct", hasReturn); 
     
      auto &sema = Clang->getSema();
      auto groupPtr = sema.ConvertDeclToDeclGroup(record);
      auto recDecl = sema.ActOnDeclStmt(groupPtr, loc, loc);

      // Create a local variable and cast void ptr to the struct type.
      clang::ParmVarDecl* voidptr = proxy->getParamDecl(0);
      // Necessary, otherwise it fails in codegen.
      markAsUsed(voidptr);
      voidptr->markUsed(*ast);
      clang::IdentifierInfo& castId = ast->Idents.get("_a_");
      auto qualType = ast->getRecordType(record);
      auto ptrQualType = ast->getPointerType(qualType);
      auto strct = clang::VarDecl::Create(
          *ast,
          proxy,
          loc,
          loc,
          &castId,
          ptrQualType,
          ast->getTrivialTypeSourceInfo(ptrQualType),
          clang::StorageClass::SC_None);
      // Necessary, otherwise fails in codegen
      markAsUsed(strct);
      strct->markUsed(*ast);
      clang::NestedNameSpecifierLoc spec1;
      auto declRef = clang::DeclRefExpr::Create(
          *ast,
          spec1,
          loc,
          voidptr,
          false,
          loc,
          voidptr->getOriginalType(),
          clang::VK_LValue);
      auto castExpr = clang::ImplicitCastExpr::Create(
          *ast,
          voidptr->getOriginalType(),
          clang::CK_LValueToRValue,
          declRef,
          nullptr,
          clang::VK_LValue,
          clang::FPOptionsOverride());
      // This might be required to avoid failing at codegen.
      castExpr->setIsPartOfExplicitCast(true);
      auto cCast = clang::CStyleCastExpr::Create(
          *ast,
          ptrQualType,
          clang::VK_LValue,
          clang::CK_BitCast,
          castExpr,
          nullptr,
          clang::FPOptionsOverride(),
          ast->getTrivialTypeSourceInfo(ptrQualType),
          strct->getLocation(),
          loc);
      strct->setInit(cCast);

      groupPtr = sema.ConvertDeclToDeclGroup(strct);
      auto varDecl = sema.ActOnDeclStmt(groupPtr, loc, loc); //clang::DeclStmt(groupPtr.get(), loc, loc);
      
      // Create arguments and type cast.
      std::vector<clang::Expr*> memberArgs;
      std::vector<clang::FieldDecl*> fields;
      for (auto f: record->fields())
      {
        fields.push_back(f);
      }

      // Check that we have the correct sizes.
      assert((target->getReturnType() == voidQual && 
            target->getNumParams() == fields.size()) ||
            (target->getNumParams() == (fields.size() -1)));
      for (unsigned i = 0; i < target->getNumParams(); i++)
      {
        auto p = target->getParamDecl(i);
        auto field = fields[i];
        clang::NestedNameSpecifierLoc spec;

        // Reference to the local variable.
        auto declRef = clang::DeclRefExpr::Create(
            *ast,
            spec,
            loc,
            strct,
            false,
            loc,
            ptrQualType,
            clang::VK_LValue);
        auto implicit = clang::ImplicitCastExpr::Create(
            *ast,
            ptrQualType,
            clang::CK_LValueToRValue,
            declRef,
            nullptr,
            clang::VK_PRValue,
            clang::FPOptionsOverride());
        auto member = clang::MemberExpr::CreateImplicit(
            *ast,
            implicit,
            true,
            field,
            p->getOriginalType(),
            clang::VK_LValue,
            clang::OK_BitField);
        auto arg = clang::ImplicitCastExpr::Create(
            *ast,
            p->getOriginalType(),
            clang::CK_LValueToRValue,
            member,
            nullptr,
            clang::VK_PRValue,
            clang::FPOptionsOverride());
        // Add the argument.
        memberArgs.push_back(arg);
      }

      // Create the function call.
      clang::DeclarationNameInfo info;
      clang::NestedNameSpecifierLoc spec;
      auto funcTy = target->getFunctionType();
      auto funcQualTy = clang::QualType(funcTy, 0);
      auto expr = clang::DeclRefExpr::Create(
          *ast,
          spec,
          loc,
          target,
          false,
          info,
          funcQualTy,
          clang::VK_LValue);

      // Implicit cast to function pointer.
      auto implCast = clang::ImplicitCastExpr::Create(
          *ast,
          ast->getPointerType(funcQualTy),
          clang::CK_FunctionToPointerDecay,
          expr,
          nullptr,
          clang::VK_PRValue,
          clang::FPOptionsOverride());

      auto call = clang::CallExpr::Create(
          *ast,
          implCast,
          memberArgs,
          target->getReturnType(),
          clang::VK_PRValue,
          loc,
          clang::FPOptionsOverride());
          
      std::vector<clang::Stmt*>lines;
      //lines.push_back(record);
      lines.push_back(recDecl.get());
      lines.push_back(varDecl.get());

      // If there is a non-void return type, assign the result to the struct.
      if (target->getReturnType() != voidQual)
      {
       clang::NestedNameSpecifierLoc spec;
       auto retField = fields.back();
       auto declRef = clang::DeclRefExpr::Create(
            *ast,
            spec,
            loc,
            strct,
            false,
            loc,
            ptrQualType,
            clang::VK_LValue);
        auto implicit = clang::ImplicitCastExpr::Create(
            *ast,
            ptrQualType,
            clang::CK_LValueToRValue,
            declRef,
            nullptr,
            clang::VK_PRValue,
            clang::FPOptionsOverride());
        auto member = clang::MemberExpr::CreateImplicit(
            *ast,
            implicit,
            true,
            retField,
            target->getReturnType(),
            clang::VK_LValue,
            clang::OK_BitField);
        auto binop = clang::BinaryOperator::Create(
            *ast, 
            member,
            call,
            clang::BO_Assign,
            target->getReturnType(),
            clang::VK_LValue,
            clang::OK_BitField,
            loc,
            clang::FPOptionsOverride());
        lines.push_back(binop);
      } else {
        lines.push_back(call);
      }
      
      auto compStmt = clang::CompoundStmt::Create(*ast, lines, loc, loc);
      proxy->setBody(compStmt);
      markAsUsed(proxy);
    }

    /**
     * Create a return instruction
     *
     * TODO: Create all the ones we use in code generation
     */
    clang::ReturnStmt*
    createReturn(clang::Expr* val, clang::FunctionDecl* func) const
    {
      auto retStmt =
        clang::ReturnStmt::Create(*ast, func->getLocation(), val, nullptr);
      func->setBody(retStmt);
      return retStmt;
    }

    /**
     * Marks a declaration as being used to prevent DCE.
     */
    void markAsUsed(clang::Decl* decl) const
    {
      assert(decl != nullptr);
      auto& S = Clang->getSema();
      clang::AttributeCommonInfo CommonInfo = {clang::SourceRange{}};
      decl->addAttr(clang::UsedAttr::Create(S.Context, CommonInfo));
    }

    /**
     * Create a call instruction with func pointer argument.
     * @warn does not set the caller body to allow for compoud statements.
     *
     */
    clang::Expr* createMemberCallFunctionArg(
      clang::CXXMethodDecl* method,
      llvm::ArrayRef<clang::ValueDecl*> args,
      clang::QualType retTy,
      clang::FunctionDecl* caller) const
    {
      clang::SourceLocation loc = caller->getLocation();
      clang::DeclarationNameInfo info;
      clang::NestedNameSpecifierLoc spec;
      auto& S = Clang->getSema();

      // Create an expression for each argument
      llvm::SmallVector<clang::Expr*, 1> argExpr;
      for (auto arg : args)
      {
        auto funcTy = arg->getFunctionType();
        auto funcQualTy = clang::QualType(funcTy, 0);
        // Get expression from declaration
        auto e = clang::DeclRefExpr::Create(
          *ast,
          spec,
          loc,
          arg,
          /*capture=*/false,
          info,
          funcQualTy,
          clang::VK_LValue);

        // Implicit cast to r-value
        auto cast = clang::ImplicitCastExpr::Create(
          *ast,
          ast->getPointerType(funcQualTy),
          clang::CK_FunctionToPointerDecay,
          e,
          /*base path=*/nullptr,
          clang::VK_PRValue,
          clang::FPOptionsOverride());
        argExpr.push_back(cast);
      }

      // Create a call to the method
      auto expr = getCXXMethodPtr(method, loc);
      auto callStmt = callCXXMethod(method, expr, argExpr, retTy, loc);

      // Mark method as used
      clang::AttributeCommonInfo CommonInfo = {clang::SourceRange{}};
      method->addAttr(clang::UsedAttr::Create(S.Context, CommonInfo));

      return callStmt;
    }

    clang::ASTContext* getASTContext() const
    {
      return ast;
    }
  };
} // namespace verona::interop
