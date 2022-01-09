#include "state.h"
#include "notation.h"
#include "visitors.h"

#include <cassert>
#include <iostream>
#include <verona.h>

using namespace verona::ir;
namespace rt = verona::rt;

namespace interpreter
{
  // TODO move this and pass it as an argument I guess.
  thread_local State state;

  void SimplePrinter::visit(NodeDef* node)
  {
    std::cout << "The type " << verona::ir::kindname(node->kind()) << std::endl;
  }

  void Interpreter::visit(NodeDef* node)
  {
    switch (node->kind())
    {
      case Kind::Var:
        evalVar(node->as<Var>());
        break;
      case Kind::Dup:
        evalDup(node->as<Dup>());
        break;
      case Kind::Load:
        evalLoad(node->as<Load>());
        break;
      case Kind::Store:
        evalStore(node->as<Store>());
        break;
      case Kind::Lookup:
        evalLookup(node->as<Lookup>());
        break;
      case Kind::Typetest:
        evalTypetest(node->as<Typetest>());
        break;
      case Kind::NewAlloc:
        evalNewAlloc(node->as<NewAlloc>());
        break;
      case Kind::StackAlloc:
        evalStackAlloc(node->as<StackAlloc>());
        break;
      case Kind::Call:
        evalCall(node->as<Call>());
        break;
      case Kind::Tailcall:
        evalTailcall(node->as<Tailcall>());
        break;
      case Kind::Region:
        evalRegion(node->as<Region>());
        break;
      case Kind::Create:
        evalCreate(node->as<Create>());
        break;
      case Kind::Branch:
        evalBranch(node->as<Branch>());
        break;
      case Kind::Return:
        evalReturn(node->as<Return>());
        break;
      case Kind::Error:
        evalError(node->as<Err>());
        break;
      case Kind::Catch:
        evalCatch(node->as<Catch>());
        break;
      case Kind::Acquire:
        evalAcquire(node->as<Acquire>());
        break;
      case Kind::Release:
        evalRelease(node->as<Release>());
        break;
      case Kind::Fulfill:
        evalFulfill(node->as<Fulfill>());
        break;
      case Kind::Freeze:
        evalFreeze(node->as<Freeze>());
      case Kind::Merge:
        evalMerge(node->as<Merge>());
      default:
        assert(0);
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
    obj->obj = rt::api::create_object(nullptr); //TODO figure out the descriptor.
    //TODO set up regions? All the regions then? 
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
    
    //x ∉ σ
    assert(!state.isDefinedInFrame(x) && "Name already defined");
    // TODO norepeat(y; v)
    //f = σ(y)
    assert(state.fields.contains(y->objectid->name) && "The object ID does not exist");
    assert(state.fields[y->objectid->name].contains(y->id->name) && "objectid does not have a field");
    auto f = state.fields[y->objectid->name][y->id->name];

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
    assert(_l->kind() == Kind::ObjectID);
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
      case Kind::Function:
        v = m;
        break;
      case Kind::StorageLoc:
        assert(state.objects[l->name]->obj->debug_is_iso() && "Lookup with an iso storageloc");
      case Kind::ObjectID:
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
    if (l->kind() != Kind::ObjectID) {
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
    obj->obj = rt::api::create_object(nullptr); //TODO figure out the descriptor.
    //TODO set up regions? 
    state.addObject(obj->id, obj);

    // σ[ι↦(ρ, τ)][x↦ι]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 
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
    string tpe = node.type->name;
    assert(!state.isDefinedInFrame(x));
    assert(state.containsType(tpe));

    // ι ∉ σ
    //σ[ι↦(σ.frame.regions, τᵩ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    obj->obj = rt::api::create_object(nullptr); //TODO figure out the descriptor.
    //TODO set up regions? All the regions then? 
    state.addObject(obj->id, obj);

    // σ[x↦ι]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 
  }

  // Push a new frame.
  // norepeat(y; z*)
  // x ∉ σ
  // σ₂, e₂* = newframe(σ₁, (), x*, y, z*, e₁*)
  // --- [call]
  // σ₁, x* = call y(z*); e₁* → σ₂, e₂*
  void Interpreter::evalCall(verona::ir::Call& node)
  {
    // x ∉ σ
    assert(node.left.size() > 0);
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }
    
    // σ₂, e₂* = newframe(σ₁, (), x*, y, z*, e₁*)
    string y = node.function->name; 
    assert(state.isFunction(y) && "Undefined Function");
    auto yfunc = state.getFunction(y);
    for (auto z: node.args) 
    {
      assert(state.isDefinedInFrame(z->name));
    }

    Shared<Frame> frame = make_shared<Frame>();
    // TODO regions
    // TODO ret
    // TODO continuations
    // TODO what does () means in terms of region?
    // TODO why is frame 1 removing y and z*?
    assert(yfunc->args.size() == node.args.size() && "Wrong number of arguments");
    for (int i = 0; i < node.args.size(); i++) {
      auto name = yfunc->args[i]->name;
      auto value = state.frameLookup(node.args[i]->name);
      frame->lookup[name] = value;
    }

    // (ϕ*; ϕ₁\{y, z*}; ϕ₂)
    state.frames.push_back(frame);

    // TODO change next instruction + call rt functions 
    // e₁* → σ₂, e₂*
  }

  // Reuse the current frame.
  // live(σ, x; y*)
  // λ = σ(x)
  // --- [tailcall]
  // σ, tailcall x(y*) → σ[λ.args↦σ(y*)], λ.expr
  void Interpreter::evalTailcall(verona::ir::Tailcall& node) {
    // live(σ, x) && λ = σ(x)
    auto x = node.function->name; 
    assert(state.isDefinedInFrame(x));
    auto _f = state.frameLookup(x);
    assert(_f->kind() == Kind::Function);
    auto lambda = dynamic_pointer_cast<ir::Function>(_f);

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
    // TODO
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
    // TODO
    // norepeat(y; z; z*)
    // x ∉ σ
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }
    
    string y = node.function->name;
    assert(state.isDefinedInFrame(y));
    auto v = state.frameLookup(y);
    assert(v->kind() == Kind::Function && "y is not a function");
    auto yfunc = dynamic_pointer_cast<ir::Function>(v);

    assert(node.args.size() > 0);
    assert(node.args.size() == yfunc->args.size());
    for (auto arg: node.args) {
      assert(state.isDefinedInFrame(arg->name));
    }
    
    // ι = σ(z)
    string z = node.args[0]->name;
    auto _l = state.frameLookup(z);
    assert(_l->kind() == Kind::ObjectID && "Argument is not an object id");
    auto l = state.getObjectByName(z);

    // ρ = σ(ι).regions
    auto regions = l->regions;

    // iso(σ, ι)
    assert(l->obj->debug_is_iso() && "z must reference an iso object");

    // σ₂, e₂* = newframe(σ₁, ρ, x*, y, (z; z*), (unpin(σ₁, z*); e₁*))
    Shared<Frame> frame = make_shared<Frame>();
    // TODO regions 
    // Is it all from the last frame + all from l?

    // [λ.args↦σ(z*)]
    for (int i = 0; i < node.args.size();i++) {
      auto name = yfunc->args[i]->name;
      auto value = state.frameLookup(node.args[i]->name);
      frame->lookup[name] = value;
    }
    
    // TODO unpin
    // TODO continuation and ret? 
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
    assert(state.isDefinedInFrame(y));
    auto v = state.frameLookup(y);
    assert(v->kind() == Kind::Function && "y is not a function");
    auto yfunc = dynamic_pointer_cast<ir::Function>(v);
  
    // ρ ∉ σ
    rt::Region* region = nullptr; //TODO

    // σ₁[ρ↦Σ]
    state.regions[region] = node.strategy;

    Shared<Frame> frame = make_shared<Frame>();
    // TODO regions 
    // (ϕ*; ϕ₁\{y, z*}; ϕ₂)
    // TODO
    //(unpin(σ₁, z*)

    // [λ.args↦σ(z*)]
    for (int i = 0; i < node.args.size();i++) {
      auto name = yfunc->args[i]->name;
      auto value = state.frameLookup(node.args[i]->name);
      frame->lookup[name] = value;
    }
    //TODO same as a call, change instruction and ret + cont
    //TODO probably factor that part out once I am sure I do newframe correctly.
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
    // live(σ₁, x; y; z; y*)
    // live(σ₁, x; y; z; z*)
    // TODO
    string x = node.condition->name;
    assert(state.isDefinedInFrame(x));

    //TODO is that correct? are they functions or should I introduce lambdas?
    string y = node.branch1->function->name;
    assert(state.isDefinedInFrame(y));
    Node<ir::Function> yfunc = state.getFunction(y);
    assert(yfunc->args.size() == node.branch1->args.size());
    string z = node.branch2->function->name;
    assert(state.isDefinedInFrame(z));
    Node<ir::Function> zfunc = state.getFunction(z);
    assert(zfunc->args.size() == node.branch2->args.size());
    
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
      //TODO update state to continue on y;
      return;
    } 
    //TODO update state to continue on z;
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
    
    bool isoOrImm = !sameRegions(phi1->regions, phi2->regions);

    assert(phi2->rets.size() == node.returns.size());
    for (int i = 0; i < phi2->rets.size(); i++) {
      auto x = node.returns[i]->name;
      auto ret = phi2->rets[i];
      assert(phi2->containsName(x) && "Return value undefined in phi2");
      auto val = phi2->lookup[x];
      assert((!isoOrImm || isIsoOrImm(state, val)) && "Value must be iso or immutable"); 
      phi1->lookup[ret] = phi2->lookup[x];
    }
    
    // Drop the last frame, i.e., phi2
    state.frames.pop_back();

    // TODO COLLECT THE OBJECTS
    // ιs = σ₁.objects(ϕ₂.regions) if ϕ₁.regions ≠ ϕ₂.regions
    // ∅ otherwise
    // σ₂\ιs
    // TODO i don't get it, we remove the objects with the same regions as phi2?
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
  void Interpreter::evalAcquire(verona::ir::Acquire& node) {}
  void Interpreter::evalRelease(verona::ir::Release& node) {}
  void Interpreter::evalFulfill(verona::ir::Fulfill& node) {}

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
    //TODO THE HELL IS V?
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

} // namespace interpreter
