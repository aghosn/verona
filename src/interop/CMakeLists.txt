find_package(Threads REQUIRED)

# We're not building LLVM, but downloading the build dir from an artefact
set(LLVM_BUILD ${VERONA_LLVM_LOCATION})

set(INTEROP_SOURCES
  src/verona-interop.cc 
  src/ASTinstr.cc 
  src/IRinstr.cc 
  src/ObjGen.cc 
  src/InteropApi.cc)

set(INTEROP_HEADERS
  include/ASTinstr.h
  include/Compiler.h
  include/CXXBuilder.h
  include/CXXInterface.h
  include/CXXQuery.h
  include/CXXType.h
  include/FS.h
  include/InteropApi.h
  include/Interop.h
  include/IRinstr.h
  include/ObjGen.h)

set(LIB_DEPENDENCIES
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

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_llvm_executable(verona-interop ${INTEROP_SOURCES})
llvm_update_compile_flags(verona-interop)

target_link_libraries(verona-interop PRIVATE 
  ${LIB_DEPENDENCIES})

install(TARGETS verona-interop RUNTIME DESTINATION .)

#add_library(interop SHARED 
#  ${INTEROP_SOURCES})
#
#set_target_properties(interop PROPERTIES PUBLIC_HEADER Interop.h)
#
#target_link_libraries(interop PUBLIC
#    clangAST
#    clangASTMatchers
#    clangAnalysis
#    clangBasic
#    clangCodeGen
#    clangDriver
#    clangEdit
#    clangFrontend
#    clangFrontendTool
#    clangLex
#    clangParse
#    clangRewrite
#    clangRewriteFrontend
#    clangSema
#    clangSerialization
#    clangTooling
#    clangCodeGen
#    LLVMX86Disassembler
#    LLVMX86AsmParser 
#    LLVMX86CodeGen 
#    LLVMX86Desc 
#    LLVMX86Info)


#TODO include all the libraries inside this one otherwise we have issues.
