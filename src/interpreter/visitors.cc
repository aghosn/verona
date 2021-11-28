#include <cassert>
#include <iostream>

#include "visitors.h"

using namespace verona::ir;

namespace interpreter {
  
void SimplePrinter::visit(NodeDef* node) {
 std::cout << "The type " << verona::ir::kindname(node->kind()) << std::endl;
}

void Interpreter::visit(NodeDef* node) {
  switch(node->kind()) {
    case Kind::Var:
      evalVar(node->as<Var>());
      break;
    case Kind::Dup:
      evalDup(node->as<Dup>());
      break;
    case Kind::Load:
      evalLoad(node->as<Load>());
      break;
    case Kind::Store:
      evalStore(node->as<Store>());
      break;
    case Kind::Lookup:
      evalLookup(node->as<Lookup>());
      break;
    case Kind::Typetest:
      evalTypetest(node->as<Typetest>());
      break;
    case Kind::NewAlloc:
      evalNewAlloc(node->as<NewAlloc>());
      break;
    case Kind::StackAlloc:
      evalStackAlloc(node->as<StackAlloc>());
      break;
    case Kind::Call:
      evalCall(node->as<Call>());
      break;
    case Kind::Tailcall:
      evalTailcall(node->as<Tailcall>());
      break;
    case Kind::Region:
      evalRegion(node->as<Region>());
      break;
    case Kind::Create:
      evalCreate(node->as<Create>());
      break;
    case Kind::Branch:
      evalBranch(node->as<Branch>());
      break;
    case Kind::Return:
      evalReturn(node->as<Return>());
      break;
    case Kind::Error:
      evalError(node->as<Err>());
      break;
    case Kind::Catch:
      evalCatch(node->as<Catch>());
      break;
    case Kind::Acquire:
      evalAcquire(node->as<Acquire>());
      break;
    case Kind::Release:
      evalRelease(node->as<Release>());
      break;
    case Kind::Fulfill:
      evalFulfill(node->as<Fulfill>());
      break;
    default:
      assert(0);
  }
}


void Interpreter::evalVar(verona::ir::Var& node){}
void Interpreter::evalDup(verona::ir::Dup& node){}
void Interpreter::evalLoad(verona::ir::Load& node){}
void Interpreter::evalStore(verona::ir::Store& node){}
void Interpreter::evalLookup(verona::ir::Lookup& node){}
void Interpreter::evalTypetest(verona::ir::Typetest& node){}
void Interpreter::evalNewAlloc(verona::ir::NewAlloc& node){}
void Interpreter::evalStackAlloc(verona::ir::StackAlloc& node){}
void Interpreter::evalCall(verona::ir::Call& node){}
void Interpreter::evalTailcall(verona::ir::Tailcall& node){}
void Interpreter::evalRegion(verona::ir::Region& node){}
void Interpreter::evalCreate(verona::ir::Create& node){}
void Interpreter::evalBranch(verona::ir::Branch& node){}
void Interpreter::evalReturn(verona::ir::Return& node){}
void Interpreter::evalError(verona::ir::Err& node){}
void Interpreter::evalCatch(verona::ir::Catch& node){}
void Interpreter::evalAcquire(verona::ir::Acquire& node){}
void Interpreter::evalRelease(verona::ir::Release& node){}
void Interpreter::evalFulfill(verona::ir::Fulfill& node){}

} // namespace interpreter
