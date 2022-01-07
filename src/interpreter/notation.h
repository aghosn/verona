#pragma once

namespace interpreter {

//live(σ, x*) = norepeat(x*) ∧ (dom(σ.frame) = dom(x*))
bool live(State& state, ir::List<ir::ID> args);

// norepeat(x*) = (|x*| = |dom(x*)|)
bool norepeat(ir::List<ir::ID> args);

bool sameRegions(List<rt::Region*> r1, List<rt::Region*> r2);

bool isIsoOrImm(State& state, Shared<ir::Value> value);

} // namespace interpreter
