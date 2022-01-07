#include "state.h"
#include "utils.h"
#include "notation.h"

namespace interpreter {

  //live(σ, x*) = norepeat(x*) ∧ (dom(σ.frame) = dom(x*))
  bool live(State& state, ir::List<ir::ID> args) {
    assert(state.frames.size() > 0 && "State has no frame!");
    bool repeat = norepeat(args);
    bool doms = state.frames.back()->lookup.size() == args.size();
    for (auto x: args) {
      doms = doms && state.isDefinedInFrame(x->name);
    }
    return repeat && doms;
  }

  // norepeat(x*) = (|x*| = |dom(x*)|)
  bool norepeat(ir::List<ir::ID> args) {
    for (int i = 0; i < args.size() -1; i++) {
      auto curr = args[i];
      for (int j = i + 1; j < args.size(); j++) {
        auto next = args[j];
        if (curr->name == next->name) {
          return false;
        }
      }
    }
    return true;
  }

  bool sameRegions(List<rt::Region*> r1, List<rt::Region*> r2) {
    if (r1.size() != r2.size()) {
      return false;
    }
    for (int i = 0; i < r1.size(); i++) {
      if (r1[i] != r2[i]) {
        return false;
      }
    }
    return true;
  }

  bool isIsoOrImm(State& state, Shared<ir::Value> value) {
    Shared<ir::ObjectID> oid = nullptr;
    Shared<Object> obj = nullptr;
    Shared<ir::StorageLoc> _v2 = nullptr;
    Shared<ir::Value> val2 = nullptr;

    switch(value->kind()) {
      case ir::Kind::ObjectID:
        oid = dynamic_pointer_cast<ir::ObjectID>(value);
        assert(state.objects.contains(oid->name));
        obj = state.objects[oid->name];
        //TODO find how to test for imm?
        return (obj->obj->debug_is_iso() /*|| obj->obj->debug_is_imm()*/);

      case ir::Kind::StorageLoc:
        _v2 = dynamic_pointer_cast<ir::StorageLoc>(value);
        assert(state.fields.contains(_v2->objectid->name));
        assert(state.fields[_v2->objectid->name].contains(_v2->id->name));
        val2 = state.fields[_v2->objectid->name][_v2->id->name];
        return isIsoOrImm(state, val2);
      
      default:
        return true;
    } 
    return false;
  }

} // namespace interpreter
