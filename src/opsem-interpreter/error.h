#pragma once

#include <iostream>
#include <exception>

using namespace std;

namespace interpreter
{
  struct InterpreterException : public exception
  {
    const char* what() const throw () 
    {
      return "Interpreter Exception!";
    }
  };

#define CHECK(condition, message) { \
  if(!(condition)) { \
    std::cerr << "[FAIL]: " << #condition << " @ " << __FILE__ << " (" << __LINE__ << ")" << std::endl;\
    std::cerr << "[HINT]: " << message << std::endl; \
    throw InterpreterException(); \
  }}
}
