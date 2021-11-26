#include "ir.h"

namespace verona::ir {

  const char* kindname(Kind k) {
    switch(k) {
      case Kind::Invalid:
        return "Invalid";
      case Kind::Identifier:
        return "Identifier";
      case Kind::ID:
        return "ID";
      case Kind::TypeID:
        return "TypeID";
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
      case Kind::Apply:
        return "Apply";
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
      case Kind::Error:
        return "Error";
      case Kind::Catch:
        return "Catch";
      case Kind::Acquire:
        return "Acquire";
      case Kind::Release:
        return "Release";
      case Kind::Fulfill:
        return "Fulfill";
      case Kind::Function:
        return "Function";
      case Kind::Iso:
        return "Iso";
      case Kind::Mut:
        return "Mut";
      case Kind::Imm:
        return "Imm";
      case Kind::Paused:
        return "Paused";
      case Kind::Stack:
        return "Stack";
      case Kind::UnionType:
        return "UniionType";
      case Kind::IsectType:
        return "IsectType";
      case Kind::TupleType:
        return "TupleType";
      case Kind::Interface:
        return "Interface";
      case Kind::Class:
        return "Class";
      case Kind::True:
        return "True";
      case Kind::False:
        return "False";
      case Kind::Undef:
        return "Undefined";
      case Kind::End:
        return "End";
      default:
        break;
    }
    return "Invalid";               
  }
  

} //   namespace verona::ir
