#include "interpreter.h"

namespace interpreter {

  Interpreter::Interpreter() {
    rules = std::vector<IRule*> {
      new VarR(),
      new DupR(),
      new LoadR(),
      new StoreR(),
      new LookupR(),
      new TypetestR(),
      new NewAllocR(),
      new CallR(),
      new RegionR(),
      new CreateR(),
      new TailcallR(),
      new BranchR(),
      new ReturnR(),
      new ErrorR(),
      new CatchR(),
      new AcquireR(),
      new ReleaseR(),
      new FulfillR(),
    };
  }

  std::vector<IRule*> Interpreter::match(Ast t) {
    std::vector<IRule*> result;
    for (auto r: rules) {
      if (r->matches(t)) {
        result.push_back(r);
      }
    }
    return result;
  }

} // namespace verona::ir;
