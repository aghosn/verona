#pragma once

#include "ir.h"

namespace interpreter {

// A simple printer for ir nodes;
class SimplePrinter: public verona::ir::Visitor {
  public:
  void visit(verona::ir::NodeDef* node);
};

// The interpreter.
class Interpreter: public verona::ir::Visitor {
  public:
  void visit(verona::ir::NodeDef* node);

  private:
  void evalVar(verona::ir::Var& node);
  void evalDup(verona::ir::Dup& node);
  void evalLoad(verona::ir::Load& node);
  void evalStore(verona::ir::Store& node);
  void evalLookup(verona::ir::Lookup& node);
  void evalTypetest(verona::ir::Typetest& node);
  void evalNewAlloc(verona::ir::NewAlloc& node);
  void evalStackAlloc(verona::ir::StackAlloc& node);
  void evalCall(verona::ir::Call& node);
  void evalTailcall(verona::ir::Tailcall& node);
  void evalRegion(verona::ir::Region& node);
  void evalCreate(verona::ir::Create& node);
  void evalBranch(verona::ir::Branch& node);
  void evalReturn(verona::ir::Return& node);
  void evalError(verona::ir::Err& node);
  void evalCatch(verona::ir::Catch& node);
  void evalAcquire(verona::ir::Acquire& node);
  void evalRelease(verona::ir::Release& node);
  void evalFulfill(verona::ir::Fulfill& node);

};

} // namespace interpreter
