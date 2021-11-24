#include "ir.h"

namespace verona::ir {

  const char* kindname(Kind k) {
    switch(k) {
      case Kind::Invalid:
        return "Invalid";
      case Kind::Var:
        return "Var";
      case Kind::Dup:
        return "Dup";
      case Kind::Load:
        return "Load";
      case Kind::Store:
        return "Store";
      case Kind::Lookup:
        return "Lookup";
      case Kind::Typetest:
        return "Typetest";
      case Kind::NewAlloc:
        return "NewAlloc";
      case Kind::StackAlloc:
        return "StackAlloc";
      case Kind::Call:
        return "Call";
      case Kind::Tailcall:
        return "Tailcall";
      case Kind::Region:
        return "Region";
      case Kind::Create:
        return "Create";
      case Kind::Branch:
        return "Branch";
      case Kind::Return:
        return "Return";
      case Kind::Acquire:
        return "Acquire";
      case Kind::Release:
        return "Release";
      case Kind::Fulfill:
        return "Fulfill";
      case Kind::Function:
        return "Function";
      case Kind::End:
        return "End";
      default:
        break;
    }
    return "Invalid";               
  }
  

} //   namespace verona::ir
