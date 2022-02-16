#pragma once
#include "state.h"
#include "utils.h"

namespace interpreter
{
  // newframe(σ, ρ*, x*, y, z*, e*) =
  // ((ϕ*; ϕ₁\{y, z*}; ϕ₂), σ.objects, σ.fields, σ.regions, σ.except), λ.expr
  // if λ ∈ Function
  // where
  //   λ = σ(y),
  //   σ.frames = (ϕ*; ϕ₁),
  //   ϕ₂ = ((ϕ₁.regions; ρ*), [λ.args↦σ(z*)], x*, e*)
  ir::List<ir::Expr> newframe(
    State& state,
    List<Region*>& p,
    ir::List<ir::ID>& xs,
    ir::Node<ir::ID> y,
    ir::List<ir::ID>& zs,
    ir::List<ir::Expr>& es);

  // live(σ, x*) = norepeat(x*) ∧ (dom(σ.frame) = dom(x*))
  bool live(State& state, ir::List<ir::ID> args);

  // norepeat(x*) = (|x*| = |dom(x*)|)
  bool norepeat(ir::List<ir::ID> args);
  bool norepeat2(ir::List<ir::ID> first, ir::Node<ir::ID> extras...);

  bool sameRegions(List<Region*> r1, List<Region*> r2);
  List<ObjectId> getObjectsInRegions(State& state, List<Region*> regions);

  bool isIsoOrImm(State& state, Shared<ir::Value> value);
  ir::List<ir::ID> removeDuplicates(ir::List<ir::ID> args);

} // namespace interpreter
