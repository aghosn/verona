find_package(Threads REQUIRED)

# We're not building LLVM, but downloading the build dir from an artefact
set(LLVM_BUILD ${VERONA_LLVM_LOCATION})

set(Clang_DIR ${LLVM_BUILD}/lib/cmake/clang)
message(STATUS "Setting Clang_DIR as ${Clang_DIR}")

find_package(Clang REQUIRED CONFIG)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(AddLLVM)

include_directories(../ds)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

add_llvm_executable(verona-interop verona-interop.cc ASTinstr.cc IRinstr.cc ObjGen.cc InteropApi.cc)
llvm_update_compile_flags(verona-interop)

target_link_libraries(verona-interop PRIVATE 
    clangAST
    clangASTMatchers
    clangAnalysis
    clangBasic
    clangCodeGen
    clangDriver
    clangEdit
    clangFrontend
    clangFrontendTool
    clangLex
    clangParse
    clangRewrite
    clangRewriteFrontend
    clangSema
    clangSerialization
    clangTooling
    clangCodeGen
    LLVMX86Disassembler
    LLVMX86AsmParser 
    LLVMX86CodeGen 
    LLVMX86Desc 
    LLVMX86Info)
#LLVM-11 apparently LLVM-11 (which is my system's default, compiles and links but fails at runtime);

install(TARGETS verona-interop RUNTIME DESTINATION .)

add_library(interop SHARED 
  verona-interop.cc
  ASTinstr.cc
  ASTinstr.h
  IRinstr.cc
  IRinstr.h
  ObjGen.cc
  ObjGen.h
  InteropApi.cc
  CXXBuilder.h
  CXXQuery.h
  FS.h
  Interop.h
  Compiler.h
  CXXInterface.h
  CXXType.h
  InteropApi.h)

set_target_properties(interop PROPERTIES PUBLIC_HEADER Interop.h)

target_link_libraries(interop PUBLIC
    clangAST
    clangASTMatchers
    clangAnalysis
    clangBasic
    clangCodeGen
    clangDriver
    clangEdit
    clangFrontend
    clangFrontendTool
    clangLex
    clangParse
    clangRewrite
    clangRewriteFrontend
    clangSema
    clangSerialization
    clangTooling
    clangCodeGen
    LLVMX86Disassembler
    LLVMX86AsmParser 
    LLVMX86CodeGen 
    LLVMX86Desc 
    LLVMX86Info)


#TODO include all the libraries inside this one otherwise we have issues.
