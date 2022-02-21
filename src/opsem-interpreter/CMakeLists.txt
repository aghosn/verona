cmake_minimum_required(VERSION 3.18)

project(opsem-interpreter)

set(CMAKE_CXX_STANDARD 20)

FILE(GLOB SOURCES
  "*.cc"
  "*.h")

include_directories("../interop/include")

add_executable(opsem-interpreter 
  ${SOURCES})

target_link_libraries(opsem-interpreter 
  verona_rt
  interop)
