#pragma once

#include "utils.h"
#include "state.h"
#include "parser.h"
#include "interoperability.h"

#include <memory>

namespace interpreter {

  /**
   * An instance of the interpreter.
   */
  class Interpreter: public ir::Visitor {
    public:
      Interpreter(ir::Parser* parser);
      /**
       * Returns false if we have more instructions, true if we are done.
       */
      bool evalOneStep();
      void eval();
      void visit(ir::NodeDef* node);
      void addSandbox(interop::SandboxConfig* sb);
    private:
      State state;
      ir::Parser* parser;

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
    void evalFreeze(verona::ir::Freeze& node);
    void evalMerge(verona::ir::Merge& node);
    void evalSandboxTrampoline(verona::ir::SandboxTrampoline& node);
  };

} //namespace interpreter
