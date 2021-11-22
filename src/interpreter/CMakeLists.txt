find_package(Threads REQUIRED)

add_library(verona-interpreter-lib
  lexer.cc
  path.cc
  escaping.cc
  source.cc)

# set(LIBS
#         verona-parser-lib
#         )

add_executable(verona-interpreter interpreter.cc)
install(TARGETS verona-interpreter RUNTIME DESTINATION .)
# target_link_libraries(verona-interpreter CLI11::CLI11)
target_link_libraries(verona-interpreter Threads::Threads)
target_link_libraries(verona-interpreter verona-interpreter-lib)
