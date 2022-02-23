#include "notation.h"

#include "state.h"
#include "utils.h"

#include <cassert>
#include <cstdarg>

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
    ir::List<ir::Expr>& es)
  {
    // λ = σ(y)
    assert(state.isFunction(y->name) && "Undefined function id");
    auto yfunc = state.getFunction(y->name);
    assert(yfunc->args.size() == zs.size() && "Wrong number of arguments");
    // σ.frames = (ϕ*; ϕ₁),
    assert(state.frames.size() > 0 && "There are no frames");
    for (auto z : zs)
    {
      assert(state.isDefinedInFrame(z->name));
    }

    //   ϕ₂ = ((ϕ₁.regions; ρ*), [λ.args↦σ(z*)], x*, e*)
    auto frame1 = state.frames.back();
    Shared<Frame> frame2 = make_shared<Frame>();
    frame2->regions.insert(
      frame2->regions.end(), frame1->regions.begin(), frame1->regions.end());
    frame2->regions.insert(frame2->regions.end(), p.begin(), p.end());
    for (int i = 0; i < zs.size(); i++)
    {
      auto name = yfunc->args[i]->name;
      auto value = state.frameLookup(zs[i]->name);
      frame2->lookup[name] = value;
    }
    for (auto x : xs)
    {
      frame2->rets.push_back(x->name);
    }
    frame2->continuations.insert(
      frame2->continuations.end(), es.begin(), es.end());

    // Use is consume.
    frame1->lookup.erase(y->name);
    for (auto z : zs)
    {
      frame1->lookup.erase(z->name);
    }
    state.frames.push_back(frame2);
    return yfunc->exprs;
  }

  // live(σ, x*) = norepeat(x*) ∧ (dom(σ.frame) = dom(x*))
  bool live(State& state, ir::List<ir::ID> args)
  {
    assert(state.frames.size() > 0 && "State has no frame!");
    bool repeat = norepeat(args);
    //TODO asked Sylvan, we need to check
    bool doms = state.frames.back()->lookup.size() == args.size();
    assert(doms);
    for (auto x : args)
    {
      doms = doms && state.isDefinedInFrame(x->name);
    }
    return repeat && doms;
  }

  // norepeat(x*) = (|x*| = |dom(x*)|)
  bool norepeat(ir::List<ir::ID> args)
  {
    if (args.size() == 0)
    {
      return true;
    }
    for (int i = 0; i < (args.size() - 1); i++)
    {
      auto curr = args[i];
      for (int j = i + 1; j < args.size(); j++)
      {
        auto next = args[j];
        if (curr->name == next->name)
        {
          return false;
        }
      }
    }
    return true;
  }

  ir::List<ir::ID> removeDuplicates(ir::List<ir::ID> args) {
    if (args.size() <= 1) {
      return args;
    }
    ir::List<ir::ID> result;
    for (auto i = 0; i < args.size(); i++)
    {
      bool unique = true;
      for (int j = i+1; j < args.size(); j++) 
      {
        if (args[i]->name == args[j]->name)
        {
          unique = false;
          break;
        }
      }
      if (unique) {
        result.push_back(args[i]);
      }
    }
    return result;
  } 

  // norepeat(x*) = (|x*| = |dom(x*)|)
  // Also supports norepeat(x; y; z*);
  bool norepeat2(ir::List<ir::ID> first, ir::Node<ir::ID> extras...)
  {
    ir::List<ir::ID> args;
    va_list varargs;
    va_start(varargs, extras);
    args.push_back(extras);
    va_end(varargs);
    args.insert(args.end(), first.begin(), first.end());

    for (int i = 0; i < args.size() - 1; i++)
    {
      auto curr = args[i];
      for (int j = i + 1; j < args.size(); j++)
      {
        auto next = args[j];
        if (curr->name == next->name)
        {
          return false;
        }
      }
    }
    return true;
  }

  bool sameRegions(List<Region*> r1, List<Region*> r2)
  {
    if (r1.size() != r2.size())
    {
      return false;
    }
    for (int i = 0; i < r1.size(); i++)
    {
      if (r1[i] != r2[i])
      {
        return false;
      }
    }
    return true;
  }

  List<ObjectId> getObjectsInRegions(State& state, List<Region*> regions)
  {
    List<ObjectId> result;
    for (auto const& x : state.objects)
    {
      if (sameRegions(regions, x.second->regions))
      {
        result.push_back(x.first);
      }
    }
    return result;
  }

  bool isIsoOrImm(State& state, Shared<ir::Value> value)
  {
    Shared<ir::ObjectID> oid = nullptr;
    Shared<Object> obj = nullptr;
    Shared<ir::StorageLoc> _v2 = nullptr;
    Shared<ir::Value> val2 = nullptr;

    switch (value->kind())
    {
      case ir::Kind::ObjectID:
        oid = dynamic_pointer_cast<ir::ObjectID>(value);
        assert(state.objects.find(oid->name) != state.objects.end());
        obj = state.objects[oid->name];
        // TODO find how to test for imm?
        return (obj->obj->debug_is_iso() /*|| obj->obj->debug_is_imm()*/);

      case ir::Kind::StorageLoc:
        _v2 = dynamic_pointer_cast<ir::StorageLoc>(value);
        assert(state.fields.find(_v2->objectid->name) != state.fields.end());
        assert(state.fields[_v2->objectid->name].find(_v2->id->name) != state.fields[_v2->objectid->name].end());
        val2 = state.fields[_v2->objectid->name][_v2->id->name];
        return isIsoOrImm(state, val2);

      default:
        return true;
    }
    return false;
  }

} // namespace interpreter
