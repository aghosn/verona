#pragma once

#include <iostream>
#include <exception>
#include <memory>
#include <string>
#include <stdexcept>
#include <cstdio>


namespace interpreter
{
  struct InterpreterException : public std::exception
  {
    const char* what() const throw () 
    {
      return "Interpreter Exception!";
    }
  };

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    auto buf = std::make_unique<char[]>( size );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

#define CHECK(condition, message) { \
  if(!(condition)) { \
    std::cerr << "[FAIL]: " << #condition << " @ " << __FILE__ << " (" << __LINE__ << ")" << std::endl;\
    std::cerr << "[HINT]: " ; \
    message; \
    std::cerr << std::endl; \
    throw InterpreterException(); \
  }}

#define E_FMT(message, ...) fprintf(stderr, message, __VA_ARGS__)
#define E_FMT2(message) fprintf(stderr, message)

/**
  * Predefined set of error messages.
  * 
  */

#define E_NAME_DEF(name) E_FMT("Name '%s' already defined in frame.", name.c_str())
#define E_NAME_NOT_DEF(name) E_FMT("Name '%s' not defined in frame.", name.c_str())
#define E_OID_NOT_DEF(oid) E_FMT("ObjectID '%s' is not defined.", oid.c_str())
#define E_MISSING_FIELD(oid, id) E_FMT("No field '%s.%s'.", oid.c_str(), id.c_str())
#define E_TYPE_NOT_DEF(name) E_FMT("Type '%s' not defined in program.", name.c_str())
#define E_MISSING_CASE E_FMT2("Unhandled case in switch statement.")
#define E_WRONG_NB(expected, got) E_FMT("Wrong number of elements, expected '%ld', got '%ld'.", (long)expected, (long)got) 
#define E_WRONG_NB_AT_LEAST(expected, got) E_FMT("Expected > %ld, got %ld", (long)expected, (long)got)
#define E_CANNOT_BE_ISO E_FMT2("Target cannot be iso.")
#define E_MUST_BE_ISO E_FMT2("Target must iso.")
#define E_WRONG_KIND(expected, got) E_FMT("Wrong kind, expected '%ld', got '%ld'.",(long)expected, (long)got)
#define E_WRONG_LOOKUP(expected, got) E_FMT("Wrong lookup, expected '%ld', got '%ld'.",(long)expected, (long)got)
#define E_NOT_STORABLE(name) E_FMT("Field '%s' is not storable.", name.c_str())
#define E_LIVELINESS E_FMT2("Problem with the liveliness.")
#define E_UNEXPECTED_VALUE E_FMT2("Unexpected value.")
#define E_STRAT_TO_REGION_TYPE E_FMT2("Unsafe strategy cannot be converted to Region Type.")
#define E_NULL E_FMT2("Value should not be null")
#define E_NON_NULL E_FMT2("Value should be null");
}
