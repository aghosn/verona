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

  void Interpreter::evalVar(verona::ir::Var& node)
  {
    assert(node.left.size() == 1);
    auto id = node.left[0];

    // Check the variable does not exist.
    // TODO: handle errors properly.
    assert((!state.isObjectDefined(id->name)) && "Name already defined");

    // The variable does not exist, we can create it.
    // TODO how do we get the type for the variable?
    // How do we specify the descriptor.
    //auto x0 = rt::api::create_object(nullptr);

    //state.addVar(id->name, x0);
    // Missing a lookup and a release of x0.
  }

  void Interpreter::evalDup(verona::ir::Dup& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    assert(!state.isObjectDefined(x) && "Name already defined");
    assert(state.isObjectDefined(y) && "Object not defined");

    Shared<Object> target = state.getObjectByName(y);
    assert(!target->obj->debug_is_iso());
    state.addInFrame(x, state.getValueByName(y));
  }

  void Interpreter::evalLoad(verona::ir::Load& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.source->name;

    assert(!state.isObjectDefined(x));
    assert(state.isObjectDefined(y));

    // TODO what is the difference with Dup? I see there is one, but I don't see
    // how to express it using the API.
    Shared<Object> target = state.getObjectByName(y);
    assert(!target->obj->debug_is_iso());
    // TODO how to get the storage location?
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
    // TODO read rules more carefully and figure that one out.
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    string z = node.z->name;

    assert(!state.isObjectDefined(x));
    assert(state.isObjectDefined(y));
    assert(state.isObjectDefined(z));

    // TODO figure out the storageloc thing.
    auto value = state.getValueByName(z);
    //TODO for y it is supposed to be a storageloc BUT how do you name a storageloc?
    state.addInFrame(x, value);
    Shared<Object> yobj = state.getObjectByName(y); 
    // TODO is y supposed to be objid.id?
    //Shared<StorageLoc> f = yobj->getStorageLoc(???);
    //remove z from frame.
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

    assert(!state.isObjectDefined(x));
    assert(state.isObjectDefined(y));
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
    assert(!state.isObjectDefined(x));
    assert(state.containsType(tpe));
    assert(state.isObjectDefined(y));

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
    string tpe = node.type->name;
    assert(!state.isObjectDefined(x));
    assert(state.containsType(tpe));

    //TypeObj* tpeObj = state.getTypeByName(tpe);

    // TODO figure that out, how do I use the type?
    // What is the difference with x = var?;
    rt::Object* obj = rt::api::create_object(nullptr);

    //state.addVar(x, obj);
  }

  void Interpreter::evalStackAlloc(verona::ir::StackAlloc& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string tpe = node.type->name;
    assert(!state.isObjectDefined(x));
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
      assert(!state.isObjectDefined(x->name));
    }

    // Check the function is defined.
    assert(state.isObjectDefined(node.function->name));
    //TODO apparently the arguments are named variables for the moment.
    //Is that correct?
    for (auto arg: node.args) {
      assert(state.isObjectDefined(arg->name));
    }
    rt::api::RegionContext::push(nullptr, rt::api::RegionContext::get_region());
    //TODO remap variables to values in the state.
    //Probably needs to create a new map, same global mappings, args.
    //TODO also change the next instruction
  }
  void Interpreter::evalTailcall(verona::ir::Tailcall& node) {
    //TODO reuses the same frame.
    assert(state.isObjectDefined(node.function->name));
    for (auto arg: node.args) {
      assert(state.isObjectDefined(arg->name));
    }
    //TODO same as above.
  }
  void Interpreter::evalRegion(verona::ir::Region& node) {
    for (auto x: node.left) {
      assert(!state.isObjectDefined(x->name));
    }
    assert(state.isObjectDefined(node.function->name));
    assert(node.args.size() > 0);
    for (auto arg: node.args) {
      assert(state.isObjectDefined(arg->name));
    }
    //TODO figure out what unpin is.
    //TODO need a new frame
    rt::api::RegionContext::push(nullptr, rt::api::RegionContext::get_region());
  } 
  void Interpreter::evalCreate(verona::ir::Create& node) {
    //TODO how do you create a region?
    for (auto x: node.left) {
      assert(!state.isObjectDefined(x->name));
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
    assert(!state.isObjectDefined(x));
    string y = node.target->name;
    assert(state.isObjectDefined(y));
    Shared<Object> target = state.getObjectByName(y);
    assert(target->obj->debug_is_iso());
    
    //rt::Object* res = rt::api::freeze(target);
    //TODO is that correct?
    //state.addVar(x, res);
  }
  void Interpreter::evalMerge(verona::ir::Merge& node) {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    assert(!state.isObjectDefined(x));
    string y = node.target->name;
    assert(state.isObjectDefined(y));
    Shared<Object> target = state.getObjectByName(y);
    assert(target->obj->debug_is_iso());
    //TODO something with the frame.
    //rt::Object* res = rt::api::merge(target);
    //TODO is that correct?
    //state.addVar(x, res);
  }

} // namespace interpreter
