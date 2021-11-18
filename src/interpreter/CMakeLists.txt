find_package(Threads REQUIRED)

set(LIBS
        verona-parser-lib
        )

add_executable(verona-interpreter verona-interpreter.cc)
install(TARGETS verona-interpreter RUNTIME DESTINATION .)
target_link_libraries(verona-interpreter CLI11::CLI11)
target_link_libraries(verona-interpreter Threads::Threads)
target_link_libraries(verona-interpreter verona-parser-lib)
