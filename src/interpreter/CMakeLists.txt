cmake_minimum_required(VERSION 3.18)

project(verona-interpreter)

set(CMAKE_CXX_STANDARD 20)

add_executable(verona-interpreter 
  main.cc
  lexer.cc
  ir.cc
  parser.cc
  helpers.cc)

# TODO(aghosn) cleanup afterwards.
#find_package(Threads REQUIRED)
#
#add_library(verona-interpreter-lib
#  lexer.cc
#  path.cc
#  escaping.cc
#  source.cc
#  ir.cc
#  parser.cc)
#
## set(LIBS
##         verona-parser-lib
##         )
#
#add_executable(verona-interpreter interpreter.cc)
#install(TARGETS verona-interpreter RUNTIME DESTINATION .)
## target_link_libraries(verona-interpreter CLI11::CLI11)
#target_link_libraries(verona-interpreter Threads::Threads)
#target_link_libraries(verona-interpreter verona-interpreter-lib)
