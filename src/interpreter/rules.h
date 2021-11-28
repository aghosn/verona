#pragma once

#include "ir.h"

namespace verona::ir {

// TODO(aghosn) change type signature for eval.
template <typename T> class Rule {
  bool matches(Ast a) { return a->kind() == T::kind(); }

  virtual bool eval(Node<T> ast);
};

class VarR : Rule<Var> {
  bool eval(Node<Var> ast) override;
};

class DupR : Rule<Dup> {
  bool eval(Node<Dup> ast) override;
};

class LoadR : Rule<Load> {
  bool eval(Node<Load> ast) override;
};

class StoreR : Rule<Store> {
  bool eval(Node<Store> ast) override;
};

class LookupR : Rule<Lookup> {
  bool eval(Node<Lookup> ast) override;
};

class TypetestR : Rule<Typetest> {
  bool eval(Node<Typetest> ast) override;
};

class NewAllocR : Rule<NewAlloc> {
  bool eval(Node<NewAlloc> ast) override;
};

class CallR : Rule<Call> {
  bool eval(Node<Call> ast) override;
};

class RegionR : Rule<Region> {
  bool eval(Node<Region> ast) override;
};

class CreateR : Rule<Create> {
  bool eval(Node<Create> ast) override;
};

class TailcallR : Rule<Tailcall> {
  bool eval(Node<Tailcall> ast) override;
};

class BranchR : Rule<Branch> {
  bool eval(Node<Branch> ast) override;
};

class ReturnR : Rule<Return> {
  bool eval(Node<Return> ast) override;
};

class ErrorR : Rule<Err> {
  bool eval(Node<Err> ast) override;
};

class CatchR : Rule<Catch> {
  bool eval(Node<Catch> ast) override;
};

class AcquireR : Rule<Acquire> {
  bool eval(Node<Acquire> ast) override;
};

class ReleaseR : Rule<Release> {
  bool eval(Node<Release> ast) override;
};

class FulfillR : Rule<Fulfill> {
  bool eval(Node<Fulfill> ast) override;
};

} // namespace verona::ir
