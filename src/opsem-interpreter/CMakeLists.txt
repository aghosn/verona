cmake_minimum_required(VERSION 3.18)

find_package(Threads REQUIRED)

# We're not building LLVM, but downloading the build dir from an artefact
set(LLVM_BUILD ${VERONA_LLVM_LOCATION})

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

set(SANDBOX_PATH "${CMAKE_SOURCE_DIR}/../experiments/process_sandbox")

add_library(sandbox SHARED IMPORTED)
set_target_properties(sandbox PROPERTIES
  IMPORTED_LOCATION "${SANDBOX_PATH}/build/libsandbox.so"
  INTERFACE_INCLUDE_DIRECTORIES "${SANDBOX_PATH}/include"
)

project(opsem-interpreter)

set(CMAKE_CXX_STANDARD 17)

FILE(GLOB SOURCES
  "*.cc"
  "*.h")

add_executable(opsem-interpreter 
  ${SOURCES})

include_directories("../interop/include")
include_directories("../../external/snmalloc/src")

target_link_libraries(opsem-interpreter 
  verona_rt
  interop
  Threads::Threads
  fmt
  rt
  sandbox
  z
  bsd
  ${LIB_DEPENDENCIES})
