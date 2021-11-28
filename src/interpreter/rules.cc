#include "rules.h"

using namespace verona::ir;

namespace interpreter {

bool VarR::eval(Node<Var> ast) {
  // TODO(aghosn)
  return false;
}

bool DupR::eval(Node<Dup> ast) { return false; }

bool LoadR::eval(Node<Load> ast) { return false; }

bool StoreR::eval(Node<Store> ast) { return false; }

bool LookupR::eval(Node<Lookup> ast) { return false; }

bool TypetestR::eval(Node<Typetest> ast) { return false; }

bool NewAllocR::eval(Node<NewAlloc> ast) { return false; }

bool CallR::eval(Node<Call> ast) { return false; }

bool RegionR::eval(Node<Region> ast) { return false; }

bool CreateR::eval(Node<Create> ast) { return false; }

bool TailcallR::eval(Node<Tailcall> ast) { return false; }

bool BranchR::eval(Node<Branch> ast) { return false; }

bool ReturnR::eval(Node<Return> ast) { return false; }

bool ErrorR::eval(Node<Err> ast) { return false; }

bool CatchR::eval(Node<Catch> ast) { return false; }

bool AcquireR::eval(Node<Acquire> ast) { return false; }

bool ReleaseR::eval(Node<Release> ast) { return false; }

bool FulfillR::eval(Node<Fulfill> ast) {return false;}

} // namespace verona::ir
