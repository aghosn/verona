cmake_minimum_required(VERSION 3.18)

project(opsem-interpreter)

set(CMAKE_CXX_STANDARD 20)

set(ALL_SOURCE_FILES 
  main.cc
  lexer.cc
  ir.cc
  parser.cc
  helpers.cc
  visitors.cc
  object.cc
  notation.cc
  interpreter.cc
  interoperability.cc)


add_executable(opsem-interpreter 
  ${ALL_SOURCE_FILES})

target_link_libraries(opsem-interpreter 
  verona_rt
  interop)
