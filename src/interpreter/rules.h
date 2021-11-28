#pragma once

#include "ir.h"

using namespace verona::ir;

namespace interpreter {

class IRule {
  public:
  bool matches(Ast a) {
    return false;
  };

  const char* print() {
    return "Invalid IRule";
  }
};

// TODO(aghosn) change type signature for eval.
template <typename T> class Rule: public IRule {
  bool matches(Ast a) { return a->kind() == T::kind();}

  const char* print() {
    return kindname(T::kind());
  }

  virtual bool eval(Node<T> ast);
};

class VarR : public Rule<Var> {
  bool eval(Node<Var> ast) override;
};

class DupR : public Rule<Dup> {
  bool eval(Node<Dup> ast) override;
};

class LoadR : public Rule<Load> {
  bool eval(Node<Load> ast) override;
};

class StoreR : public Rule<Store> {
  bool eval(Node<Store> ast) override;
};

class LookupR : public Rule<Lookup> {
  bool eval(Node<Lookup> ast) override;
};

class TypetestR : public Rule<Typetest> {
  bool eval(Node<Typetest> ast) override;
};

class NewAllocR : public Rule<NewAlloc> {
  bool eval(Node<NewAlloc> ast) override;
};

class CallR : public Rule<Call> {
  bool eval(Node<Call> ast) override;
};

class RegionR : public Rule<Region> {
  bool eval(Node<Region> ast) override;
};

class CreateR : public Rule<Create> {
  bool eval(Node<Create> ast) override;
};

class TailcallR : public Rule<Tailcall> {
  bool eval(Node<Tailcall> ast) override;
};

class BranchR : public Rule<Branch> {
  bool eval(Node<Branch> ast) override;
};

class ReturnR : public Rule<Return> {
  bool eval(Node<Return> ast) override;
};

class ErrorR : public Rule<Err> {
  bool eval(Node<Err> ast) override;
};

class CatchR : public Rule<Catch> {
  bool eval(Node<Catch> ast) override;
};

class AcquireR : public Rule<Acquire> {
  bool eval(Node<Acquire> ast) override;
};

class ReleaseR : public Rule<Release> {
  bool eval(Node<Release> ast) override;
};

class FulfillR : public Rule<Fulfill> {
  bool eval(Node<Fulfill> ast) override;
};

} // namespace interpreter
