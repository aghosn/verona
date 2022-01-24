#include "interpreter.h"

#include "state.h"
#include "notation.h"
#include "utils.h"

#include <cassert>
#include <iostream>
#include <verona.h>

using namespace std;

namespace interpreter {

  static const char* ENTRY_POINT = "main";
  static const int64_t _PC_RESET = -1;
  static const int64_t _PC_START = 0;

Interpreter::Interpreter(ir::Parser parser) {
  state.init(parser.classes, parser.functions);
  //TODO set up the entry point?
  if (state.isFunction(ENTRY_POINT)) {
    auto entry = state.getFunction(ENTRY_POINT);
    state.exec_state = {entry->exprs, _PC_START};
  }
  // Setup the fake first frame and the main frame.
  // TODO: figure out whether we need more info in there.
  auto fakeframe = make_shared<Frame>();
  auto mainframe = make_shared<Frame>();
  state.frames.push_back(fakeframe);
  state.frames.push_back(mainframe);

  // Create original region
  auto mainRegion = new (rt::RegionType::Arena) Region;
  state.regions[mainRegion] = rt::RegionType::Arena;
  mainframe->regions.push_back(mainRegion);
  rt::api::open_region(mainRegion);
}

  bool Interpreter::evalOneStep() {
    static int counter = 0;
    // TODO Safety checks, we'll see if we keep or replace them. 
    assert(state.exec_state.offset >= 0 && "Invalid PC offset");
    assert((state.exec_state.offset == 0 || 
        state.exec_state.offset < state.exec_state.exprs.size())
        && "PC incompatible with the set of expressions available");

    // Stopping condition.
    if (state.exec_state.exprs.size() == 0)
    {
      // Check that the only frame available is the fake one.
      assert(state.frames.size() == 1);
      return true;
    }

    auto pc = state.exec_state.offset;
    auto instr = state.exec_state.exprs[pc];
    // TODO register some debugging information with the above.
    cout << "[EVAL]: nb_instr: " << counter << " offset: " << pc << endl;
    instr->accept(this);

    // increase pc 
    state.exec_state.offset++;

    counter++;
    return false;
  }

  void Interpreter::eval() {
    // TODO is that the correct condition?
    bool done = false;
    while((!state.except) && !done)
    {
      done = evalOneStep();
    }
    cout << "[EVAL]: done" << endl;
  }

  void Interpreter::visit(ir::NodeDef* node)
  {
    switch (node->kind())
    {
      case ir::Kind::Var:
        evalVar(node->as<ir::Var>());
        break;
      case ir::Kind::Dup:
        evalDup(node->as<ir::Dup>());
        break;
      case ir::Kind::Load:
        evalLoad(node->as<ir::Load>());
        break;
      case ir::Kind::Store:
        evalStore(node->as<ir::Store>());
        break;
      case ir::Kind::Lookup:
        evalLookup(node->as<ir::Lookup>());
        break;
      case ir::Kind::Typetest:
        evalTypetest(node->as<ir::Typetest>());
        break;
      case ir::Kind::NewAlloc:
        evalNewAlloc(node->as<ir::NewAlloc>());
        break;
      case ir::Kind::StackAlloc:
        evalStackAlloc(node->as<ir::StackAlloc>());
        break;
      case ir::Kind::Call:
        evalCall(node->as<ir::Call>());
        break;
      case ir::Kind::Tailcall:
        evalTailcall(node->as<ir::Tailcall>());
        break;
      case ir::Kind::Region:
        evalRegion(node->as<ir::Region>());
        break;
      case ir::Kind::Create:
        evalCreate(node->as<ir::Create>());
        break;
      case ir::Kind::Branch:
        evalBranch(node->as<ir::Branch>());
        break;
      case ir::Kind::Return:
        evalReturn(node->as<ir::Return>());
        break;
      case ir::Kind::Error:
        evalError(node->as<ir::Err>());
        break;
      case ir::Kind::Catch:
        evalCatch(node->as<ir::Catch>());
        break;
      case ir::Kind::Acquire:
        evalAcquire(node->as<ir::Acquire>());
        break;
      case ir::Kind::Release:
        evalRelease(node->as<ir::Release>());
        break;
      case ir::Kind::Fulfill:
        evalFulfill(node->as<ir::Fulfill>());
        break;
      case ir::Kind::Freeze:
        evalFreeze(node->as<ir::Freeze>());
      case ir::Kind::Merge:
        evalMerge(node->as<ir::Merge>());
      default:
        assert(0 && "Unknown node");
    }
  }

  // x ∉ σ
  // ι ∉ σ
  // --- [var]
  // σ, x = var; e* → σ[ι↦(σ.frame.regions, τᵩ)][x↦(ι, x)], acquire x; e*
  void Interpreter::evalVar(verona::ir::Var& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;

    // x ∉ σ
    assert((!state.isDefinedInFrame(x)) && "Name already defined");
    assert(state.containsType(node.type->name) && "Undefined type");

    // ι ∉ σ
    //σ[ι↦(σ.frame.regions, τᵩ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    obj->obj = new VObject(); 
    // Setting the object's regions
    for (auto r: state.frames.back()->regions) {
      obj->regions.push_back(r);
    } 
    state.addObject(obj->id, obj);

    // σ[ι↦(σ.frame.regions, τᵩ)][x↦(ι, x)]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 

    // acquire x
    //TODO
  }

  // x ∉ σ
  // ¬iso(σ, σ(y))
  // --- [dup]
  // σ, x = dup y; e* → σ[x↦σ(y)], acquire x; e*
  void Interpreter::evalDup(verona::ir::Dup& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;

    // x ∉ σ
    assert(!state.isDefinedInFrame(x) && "Name already defined");
   
    // ¬iso(σ, σ(y))
    assert(state.isDefinedInFrame(y) && "Object not defined");
    auto yvalue = state.frameLookup(y);
    Shared<Object> target = state.getObjectByName(y);
    assert(!target->obj->debug_is_iso());
    
    // σ[x↦σ(y)]
    state.addInFrame(x, state.getValueByName(y));

    // acquire x
    // TODO 
  }

  // x ∉ σ
  // f = σ(y)
  // ¬iso(σ, σ(f))
  // --- [load]
  // σ, x = load y; e* → σ[x↦σ(f)], acquire x; e*
  void Interpreter::evalLoad(verona::ir::Load& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.source->name;

    // x ∉ σ
    assert(!state.isDefinedInFrame(x));

    // f = σ(y)
    // ¬iso(σ, σ(f))
    assert(state.isDefinedInFrame(y));
    auto f = state.frameLookup(y);
    assert(f->kind() == ir::Kind::StorageLoc);

    auto storage = dynamic_pointer_cast<ir::StorageLoc>(f);
    ObjectId oid = storage->objectid->name;
    Id id = storage->id->name;
    assert(state.objects.contains(oid));
    assert(state.fields.contains(oid) && state.fields[oid].contains(id));
    
    auto value = state.fields[oid][id]; 
    // TODO check if this value corresponds to an iso object.

    //  σ[x↦σ(f)]
    state.addInFrame(x, value);

    // acquire x
    // TODO
  }

  // Load a StorageLoc and replace its content in a single step.
  //x ∉ σ
  //norepeat(y; z)
  //f = σ(y)
  //v = σ(z)
  //store(σ, f.id, v)
  //--- [store]
  //σ, x = store y z; e* → σ[f↦v][x↦σ(f)]\{z}, e*
  void Interpreter::evalStore(verona::ir::Store& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    ir::Node<ir::StorageLoc> y = node.y;
    string z = node.z->name;

    // TODO Should we check if the field is storable? 
    
    //x ∉ σ
    assert(!state.isDefinedInFrame(x) && "Name already defined");
    // TODO norepeat
    //f = σ(y)
    assert(state.isDefinedInFrame(y->objectid->name) && "The object is not defined");
    auto yoid = dynamic_pointer_cast<ir::ObjectID>(state.getValueByName(y->objectid->name)); 
    assert(state.fields.contains(yoid->name) && "The object ID does not exist");
    assert(state.fields[yoid->name].contains(y->id->name) && "objectid does not have a field");
    auto f = state.fields[yoid->name][y->id->name];

    //v = σ(z)
    assert(state.isDefinedInFrame(z) && "Source of store not defined");
    auto v = state.frameLookup(z);
    
    // σ[f↦v]
    state.fields[y->objectid->name][y->id->name] = v;

    // [x↦σ(f)]
    state.addInFrame(x, f);
    
    // σ\{z}
    state.removeFromFrame(z);
  }

  // Look in the descriptor table of an object.
  // We can't lookup a StorageLoc unless the object is not iso.
  //x ∉ σ
  //ι = σ(y)
  //m = σ(ι)(z)
  //v = (ι, m) if m ∈ Id
  //    m if m ∈ Function
  //v ∈ StorageLoc ⇒ ¬iso(σ, ι)
  //--- [lookup]
  //σ, x = lookup y z; e* → σ[x↦v], acquire x; e*
  void Interpreter::evalLookup(verona::ir::Lookup& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    string z = node.z->name;

    //x ∉ σ
    assert(!state.isDefinedInFrame(x));

    //ι = σ(y)
    assert(state.isDefinedInFrame(y));
    auto _l = state.getValueByName(y);
    assert(_l->kind() == ir::Kind::ObjectID);
    auto l = dynamic_pointer_cast<ir::ObjectID>(_l);
    
    // m = σ(ι)(z)
    // v = (ι, m)
    assert(state.fields.contains(l->name) && "The objectId is invalid");
    assert(state.fields[l->name].contains(z) && "There is no such field in the object");
    auto m = state.fields[l->name][z];

    // v = (ι, m) if m ∈ Id
    //     m if m ∈ Function
    // v ∈ StorageLoc ⇒ ¬iso(σ, ι)
    Shared<ir::Value> v = nullptr;
    switch(m->kind()) {
      case ir::Kind::Function:
        v = m;
        break;
      case ir::Kind::StorageLoc:
        assert(state.objects[l->name]->obj->debug_is_iso() && "Lookup with an iso storageloc");
      case ir::Kind::ObjectID:
       v = m; 
       break;
      default:
        assert(0 && "Can a lookup map to a bool or undefined?");
    }

    // σ[x↦v]
    state.addInFrame(x, v);

    //TODO acquire x
  }


  // Check abstract subtyping.
  // TODO: stuck if not an object?
  //x ∉ σ
  //v = σ(ι) <: τ if ι ∈ ObjectId where ι = σ(y)
  //    false otherwise
  //--- [typetest]
  //σ, x = typetest y τ; e* → σ[x↦v], e*
  void Interpreter::evalTypetest(verona::ir::Typetest& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    string tpe = node.type->name;

    // x ∉ σ
    assert(!state.isDefinedInFrame(x));
    assert(state.containsType(tpe));
    assert(state.isDefinedInFrame(y));

    // v = σ(ι) <: τ if ι ∈ ObjectId where ι = σ(y)
    Shared<ir::Value> res = make_shared<ir::False>();
    Shared<Object> yobj = nullptr;
    Shared<ir::ObjectID> oid = nullptr;
    auto l = state.frameLookup(y);
    if (l->kind() != ir::Kind::ObjectID) {
      goto end;
    }
    oid = dynamic_pointer_cast<ir::ObjectID>(l);
    if (!state.objects.contains(oid->name))
    {
      goto end;
    }
    yobj = state.objects[oid->name];
    if (yobj->type == tpe) {
      res = make_shared<ir::True>();
    }
    
end:
    state.addInFrame(x, res);
  }

  // Create a new object in the current open region, i.e. a heap object.
  // All fields are initially undefined.
  //x ∉ σ
  //ι ∉ σ
  //σ.frame.regions = (ρ*; ρ)
  //--- [new]
  //σ, x = new τ; e* → σ[ι↦(ρ, τ)][x↦ι], e*
  void Interpreter::evalNewAlloc(verona::ir::NewAlloc& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string type = node.type->name;

    // x ∉ σ
    assert(!state.isDefinedInFrame(x));
    assert(state.containsType(type));
    
    // ι ∉ σ
    // σ[ι↦(ρ, τ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    //TODO figure out the descriptor.
    obj->obj = new VObject(); 
    // Set up regions
    for (auto r: state.frames.back()->regions) {
      obj->regions.push_back(r);
    }
    state.addObject(obj->id, obj);

    // σ[ι↦(ρ, τ)][x↦ι]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 

    // All fields are initially undefined.
    auto tpe = state.getTypeByName(type);
    for (auto f: tpe->members) {
      state.fields[oid->name][f.first] = make_shared<ir::Undef>();
    }
  }

  // Create a new object in all open regions, i.e. a stack object.
  // All fields are initially undefined.
  // x ∉ σ
  // ι ∉ σ
  // --- [stack]
  // σ, x = stack τ; e* → σ[ι↦(σ.frame.regions, τ)][x↦ι], e*
  void Interpreter::evalStackAlloc(verona::ir::StackAlloc& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string type = node.type->name;
    assert(!state.isDefinedInFrame(x));
    assert(state.containsType(type));

    // ι ∉ σ
    //σ[ι↦(σ.frame.regions, τᵩ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    obj->obj = new VObject(); 
    // Set up regions
    for (auto r: state.frames.back()->regions) {
      obj->regions.push_back(r);
    }
    state.addObject(obj->id, obj);

    // σ[x↦ι]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 

    // All fields are initially undefined.
    auto tpe = state.getTypeByName(type);
    for (auto f: tpe->members) {
      state.fields[oid->name][f.first] = make_shared<ir::Undef>();
    }
  }

  // Push a new frame.
  // norepeat(y; z*)
  // x ∉ σ
  // σ₂, e₂* = newframe(σ₁, (), x*, y, z*, e₁*)
  // --- [call]
  // σ₁, x* = call y(z*); e₁* → σ₂, e₂*
  void Interpreter::evalCall(verona::ir::Call& node)
  {
    // norepeat(y; z*)
    norepeat2(node.args, node.function);
    // x ∉ σ
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }
    auto conts = state.exec_state.getContinuation();
    List<Region*> p;
    auto expr2 = newframe(state, p, node.left, 
        node.function, node.args, conts);
    state.exec_state = {expr2, _PC_RESET};
  }

  // Reuse the current frame.
  // live(σ, x; y*)
  // λ = σ(x)
  // --- [tailcall]
  // σ, tailcall x(y*) → σ[λ.args↦σ(y*)], λ.expr
  void Interpreter::evalTailcall(verona::ir::Tailcall& node) {
    // live(σ, x) && λ = σ(x)
    auto x = node.function->name; 
    auto lambda = state.getFunction(x);

    // live(σ, y*)
    assert(node.args.size() == lambda->args.size());
    for (auto y: node.args) {
      assert(state.isDefinedInFrame(y->name));
    }

    // σ[λ.args↦σ(y*)]
    for (int i = 0; i < node.args.size(); i++) {
      auto name = lambda->args[i]->name;
      auto val = state.frameLookup(node.args[i]->name);
      state.addInFrame(name, val); 
    } 

    // λ.expr
    state.exec_state = {lambda->exprs, _PC_RESET}; 
  }

  // Push a new frame with the specified heap region.
  // norepeat(y; z; z*)
  // x ∉ σ
  // ι = σ(z)
  // ρ = σ(ι).regions
  // iso(σ, ι)
  // σ₂, e₂* = newframe(σ₁, ρ, x*, y, (z; z*), (unpin(σ₁, z*); e₁*))
  // --- [region]
  // σ₁, x* = region y(z, z*); e₁* → σ₂, e₂*
  void Interpreter::evalRegion(verona::ir::Region& node) {
    // norepeat(y; z; z*)
    norepeat2(node.args, node.function);
    // x ∉ σ
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }
    
    string y = node.function->name;
    auto yfunc = state.getFunction(y);

    assert(node.args.size() > 0);
    assert(node.args.size() == yfunc->args.size());
    for (auto arg: node.args) {
      assert(state.isDefinedInFrame(arg->name));
    }
    
    // ι = σ(z)
    string z = node.args[0]->name;
    auto _l = state.frameLookup(z);
    assert(_l->kind() == ir::Kind::ObjectID && "Argument is not an object id");
    auto l = state.getObjectByName(z);

    // ρ = σ(ι).regions
    auto regions = l->regions;

    // iso(σ, ι)
    assert(l->obj->debug_is_iso() && "z must reference an iso object");

    // TODO (unpin(σ₁, z*))
    auto conts = state.exec_state.getContinuation();
    auto expr2 = newframe(state, regions, node.left,
        node.function, node.args, conts);
    state.exec_state = {expr2, _PC_RESET};
  } 

  // Create a new heap region.
  // x ∉ σ
  // ρ ∉ σ
  // σ₂, e₂* = newframe(σ₁[ρ↦Σ], ρ, x*, y, z*, (unpin(σ₁, z*); e₁*))
  // --- [create]
  // σ, x* = create Σ y(z*); e* → σ₂, e₂*
  void Interpreter::evalCreate(verona::ir::Create& node) {
    // x ∉ σ
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }

    // get y
    string y = node.function->name;
    auto yfunc = state.getFunction(y);
  
    // ρ ∉ σ
    //TODO I have to find out what to do here, typecast to my own type?
    Region* region = new (node.strategy) Region; 
    
    // σ₁[ρ↦Σ]
    state.regions[region] = node.strategy;

    auto conts = state.exec_state.getContinuation();
    List<Region*> p;
    p.push_back(region);
    //TODO do: unpin(σ₁, z*) 
    auto expr2 = newframe(state, p, node.left, node.function, node.args, conts);
    state.exec_state = {expr2, _PC_RESET};
  }

  // live(σ₁, x; y; z; y*)
  // live(σ₁, x; y; z; z*)
  // λ₁ = σ₁(y)
  // λ₂ = σ₁(z)
  // σ₂, e* = σ₁[λ₁.args↦σ₁(y*)], λ₁.expr if σ₁(x) = true
  //          σ₁[λ₂.args↦σ₁(z*)], λ₂.expr if σ₁(x) = false
  // --- [branch]
  // σ₁, branch x y(y*) z(z*) → σ₂, e*
  void Interpreter::evalBranch(verona::ir::Branch& node) {
    string x = node.condition->name;
    assert(state.isDefinedInFrame(x));
    string y = node.branch1->function->name;
    ir::Node<ir::Function> yfunc = state.getFunction(y);
    assert(yfunc->args.size() == node.branch1->args.size());
    string z = node.branch2->function->name;
    ir::Node<ir::Function> zfunc = state.getFunction(z);
    assert(zfunc->args.size() == node.branch2->args.size());

    // live(σ₁, x; y; z; y*)
    ir::List<ir::ID> namesY {node.condition, node.branch1->function, node.branch2->function};
    namesY.insert(namesY.end(), node.branch1->args.begin(), node.branch1->args.end());
    live(state, namesY);
    // live(σ₁, x; y; z; z*)
    ir::List<ir::ID> namesZ {node.condition, node.branch1->function, node.branch2->function};
    namesZ.insert(namesZ.end(), node.branch2->args.begin(), node.branch2->args.end());
    live(state, namesZ);
    
    
    //TODO factor check call is well formed.
    ir::Node<ir::Value> xval = state.frameLookup(x);
    bool condition = true;
    switch (xval->kind()) {
      case ir::Kind::True:
        condition = true;
        break;
      case ir::Kind::False:
        condition = false;
        break;
      default:
        assert(0 && "Branch malformed, x is not a boolean"); 
    }
    if (condition) {
      // TODO how do you get back to previous continuation?
      // i.e., if there is code after the branch
      state.exec_state = {yfunc->exprs, _PC_RESET};
      return;
    } 
    state.exec_state = {zfunc->exprs, _PC_RESET};
  }

  // Pop the current frame.
  // Can only return iso or imm across a `using`.
  // TODO: the isos being returned have to be disjoint
  // live(σ₁, x*)
  // σ₁.frames = (ϕ*; ϕ₁; ϕ₂)
  // σ₂ = ((ϕ*; ϕ₁[ϕ₂.ret↦σ₁(x*)]), σ₁.objects, σ₁.fields, σ₁.except)
  // (ϕ₁.regions ≠ ϕ₂.regions) ⇒ iso(σ₂, ϕ₂(x)) ∨ imm(σ₂, ϕ₂(x))
  // ιs = σ₁.objects(ϕ₂.regions) if ϕ₁.regions ≠ ϕ₂.regions
  //      ∅ otherwise
  // --- [return]
  // σ₁, return x* → σ₂\ιs, ϕ₂.cont
  void Interpreter::evalReturn(verona::ir::Return& node) {
    assert(live(state, node.returns)); 
    // σ₁.frames = (ϕ*; ϕ₁; ϕ₂)
    assert(state.frames.size() >= 2);
    // σ₂ = ((ϕ*; ϕ₁[ϕ₂.ret↦σ₁(x*)]), σ₁.objects, σ₁.fields, σ₁.except)
    auto phi1 = state.frames[state.frames.size() - 2];
    auto phi2 = state.frames[state.frames.size() - 1];
    
    bool samereg = sameRegions(phi1->regions, phi2->regions);

    // Discard return values or assign them.
    assert(phi2->rets.size() == node.returns.size());
    for (int i = 0; i < phi2->rets.size(); i++) {
      auto x = node.returns[i]->name;
      auto ret = phi2->rets[i];
      assert(phi2->containsName(x) && "Return value undefined in phi2");
      auto val = phi2->lookup[x];
      assert((samereg || isIsoOrImm(state, val)) && "Value must be iso or immutable"); 
      phi1->lookup[ret] = phi2->lookup[x];
    }
    
    // Drop the last frame, i.e., phi2
    state.frames.pop_back();

    // ιs = σ₁.objects(ϕ₂.regions) if ϕ₁.regions ≠ ϕ₂.regions
    // ∅ otherwise
    // σ₂\ιs
    if (!samereg) {
      List<ObjectId> ls = getObjectsInRegions(state, phi2->regions);
      for (auto l: ls) {
        state.objects.erase(l);
      }
    }
    state.exec_state = {phi2->continuations, _PC_RESET};
  }

  // Unset the success flag.
  // --- [error]
  // σ, error; e* → (σ.frames, σ.objects, σ.fields, σ.regions, false), e*
  void Interpreter::evalError(verona::ir::Err& node) {
    state.success = false;
  }

  // Test and set the success flag.
  // x ∉ σ₁
  // σ₂ = (σ.frames, σ.objects, σ.fields, σ.regions, true)
  // --- [catch]
  // σ₁, x = catch; e* → σ₂[x↦σ₁.except], e*
  void Interpreter::evalCatch(verona::ir::Catch& node) {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    assert(!state.isDefinedInFrame(x) && "Name already defined");
    assert(state.except == true);
    auto val = make_shared<ir::True>();
    state.addInFrame(x, val); 
    //TODO is dat it?
  }

  // TODO: rcinc
  // address: ?
  // object: ?
  // ι = ϕ(x)
  // ω = χ(ι) => ???
  // --- [acquire]
  // σ, acquire x; e* → σ, e*
  void Interpreter::evalAcquire(verona::ir::Acquire& node) {
    string x = node.target->name;
    assert(state.isDefinedInFrame(x));
    auto value = state.frameLookup(x);
    assert(value->kind() == ir::Kind::ObjectID);
    auto oid = dynamic_pointer_cast<ir::ObjectID>(value);
    assert(state.objects.contains(oid->name));
    // TODO ???
  }

  // TODO:
  // if ι is on the stack, destroy it
  // if ι is from an object, rcdec the object
  // ι = ϕ(x)
  // --- [release]
  // σ, release x; e* → σ, e*
  void Interpreter::evalRelease(verona::ir::Release& node) {
    string x = node.target->name;
    assert(state.isDefinedInFrame(x));
    // TODO how do we release it?
    state.removeFromFrame(x);
    // TODO Should remove from objects??
  }

  // TODO: fulfill the promise
  // --- [fulfill]
  // σ, fulfill x → σ, ∅
  void Interpreter::evalFulfill(verona::ir::Fulfill& node) {
    //TODO ???
    //It's an incref basically
  }

  // Destroy a region and freeze all objects that were in it.
  // x ∉ σ
  // ι = σ(y)
  // iso(σ, ι)
  // --- [freeze]
  // σ, x = freeze y; e* → σ[σ(v).regions→∅]\{y}[x↦v], acquire x; e*
  void Interpreter::evalFreeze(verona::ir::Freeze& node) {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    assert(!state.isDefinedInFrame(x));
    string y = node.target->name;
    assert(state.isDefinedInFrame(y));
    Shared<Object> target = state.getObjectByName(y);
    assert(target->obj->debug_is_iso());
    
    //rt::Object* res = rt::api::freeze(target);
    //TODO THE HELL IS V? Should be l
  }
  void Interpreter::evalMerge(verona::ir::Merge& node) {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    assert(!state.isDefinedInFrame(x));
    string y = node.target->name;
    assert(state.isDefinedInFrame(y));
    Shared<Object> target = state.getObjectByName(y);
    assert(target->obj->debug_is_iso());
    //TODO something with the frame.
    //rt::Object* res = rt::api::merge(target);
    //TODO is that correct?
    //state.addVar(x, res);
  }


} //namespace interpreter
