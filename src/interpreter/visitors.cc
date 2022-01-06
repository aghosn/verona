#include "visitors.h"

#include "state.h"

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

    assert(!state.isDefinedInFrame(x));
    assert(state.isDefinedInFrame(y));
    Shared<Object> yobj = state.getObjectByName(y);

    Shared<ir::Value> v = nullptr;
    if (yobj->fields.contains(z)) {
      v = yobj->getStorageLoc(z);
    } else {
      //TODO This is a function, how the hell do you get a function? 
    }
    state.addInFrame(x, v);
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
    assert(!state.isDefinedInFrame(x));
    assert(state.containsType(tpe));
    assert(state.isDefinedInFrame(y));

    Shared<Object> yobj = state.getObjectByName(y);
    Shared<ir::Value> res = nullptr;
    if (yobj->type == tpe) {
      res = make_shared<ir::True>();
    } else {
      res = make_shared<ir::False>();
    }
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

  void Interpreter::evalStackAlloc(verona::ir::StackAlloc& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string tpe = node.type->name;
    assert(!state.isDefinedInFrame(x));
    assert(state.containsType(tpe));

    //TypeObj* tpeObj = state.getTypeByName(tpe);

    // TODO Again figure that out;
    rt::Object* obj = rt::api::create_object(nullptr);
    //state.addVar(x, obj);
  }

  void Interpreter::evalCall(verona::ir::Call& node)
  {
    // TODO requires to define a frame.
    // Something like
    assert(node.left.size() > 0);
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }

    // Check the function is defined.
    assert(state.isDefinedInFrame(node.function->name));
    //TODO apparently the arguments are named variables for the moment.
    //Is that correct?
    for (auto arg: node.args) {
      assert(state.isDefinedInFrame(arg->name));
    }
    rt::api::RegionContext::push(nullptr, rt::api::RegionContext::get_region());
    //TODO remap variables to values in the state.
    //Probably needs to create a new map, same global mappings, args.
    //TODO also change the next instruction
  }
  void Interpreter::evalTailcall(verona::ir::Tailcall& node) {
    //TODO reuses the same frame.
    assert(state.isDefinedInFrame(node.function->name));
    for (auto arg: node.args) {
      assert(state.isDefinedInFrame(arg->name));
    }
    //TODO same as above.
  }
  void Interpreter::evalRegion(verona::ir::Region& node) {
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }
    assert(state.isDefinedInFrame(node.function->name));
    assert(node.args.size() > 0);
    for (auto arg: node.args) {
      assert(state.isDefinedInFrame(arg->name));
    }
    //TODO figure out what unpin is.
    //TODO need a new frame
    rt::api::RegionContext::push(nullptr, rt::api::RegionContext::get_region());
  } 
  void Interpreter::evalCreate(verona::ir::Create& node) {
    //TODO how do you create a region?
    for (auto x: node.left) {
      assert(!state.isDefinedInFrame(x->name));
    }
  }
  void Interpreter::evalBranch(verona::ir::Branch& node) {}
  void Interpreter::evalReturn(verona::ir::Return& node) {}
  void Interpreter::evalError(verona::ir::Err& node) {}
  void Interpreter::evalCatch(verona::ir::Catch& node) {}
  void Interpreter::evalAcquire(verona::ir::Acquire& node) {}
  void Interpreter::evalRelease(verona::ir::Release& node) {}
  void Interpreter::evalFulfill(verona::ir::Fulfill& node) {}

  void Interpreter::evalFreeze(verona::ir::Freeze& node) {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    assert(!state.isDefinedInFrame(x));
    string y = node.target->name;
    assert(state.isDefinedInFrame(y));
    Shared<Object> target = state.getObjectByName(y);
    assert(target->obj->debug_is_iso());
    
    //rt::Object* res = rt::api::freeze(target);
    //TODO is that correct?
    //state.addVar(x, res);
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
