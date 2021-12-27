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
    assert((!state.containsVariable(id->name)) && "Name already defined");

    // The variable does not exist, we can create it.
    // TODO how do we get the type for the variable?
    // How do we specify the descriptor.
    auto x0 = rt::api::create_object(nullptr);

    state.addVar(id->name, x0);
    // Missing a lookup and a release of x0.
  }

  void Interpreter::evalDup(verona::ir::Dup& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    assert(!state.containsVariable(x));
    assert(state.containsVariable(y));

    rt::Object* target = state.getVarByName(y);
    assert(!target->debug_is_iso());
    state.addVar(x, target);
  }

  void Interpreter::evalLoad(verona::ir::Load& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.source->name;

    assert(!state.containsVariable(x));
    assert(state.containsVariable(y));

    // TODO what is the difference with Dup? I see there is one, but I don't see
    // how to express it using the API.
    rt::Object* target = state.getVarByName(y);
    assert(!target->debug_is_iso());
    // TODO how to get the storage location?
  }

  void Interpreter::evalStore(verona::ir::Store& node)
  {
    // TODO read rules more carefully and figure that one out.
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    string z = node.z->name;

    assert(!state.containsVariable(x));
    assert(state.containsVariable(y));
    assert(state.containsVariable(z));

    // TODO figure out the storageloc thing.
    // probably will get the objects and do *y = *z
  }

  void Interpreter::evalLookup(verona::ir::Lookup& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;

    assert(!state.containsVariable(x));
    assert(state.containsVariable(y));
    rt::Object* yobj = state.getVarByName(y);
    // TODO get the descriptor of y, lookup for member z?
  }

  void Interpreter::evalTypetest(verona::ir::Typetest& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string y = node.y->name;
    string tpe = node.type->name;
    assert(!state.containsVariable(x));
    assert(state.containsType(tpe));

    if (!state.containsVariable(y))
    {
      // TODO set x to false and return
      return;
    }

    rt::Object* yobj = state.getVarByName(y);
    TypeObj* tpeObj = state.getTypeByName(tpe);
    // TODO do the type test and set the result.
  }

  void Interpreter::evalNewAlloc(verona::ir::NewAlloc& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string tpe = node.type->name;
    assert(!state.containsVariable(x));
    assert(state.containsType(tpe));

    TypeObj* tpeObj = state.getTypeByName(tpe);

    // TODO figure that out, how do I use the type?
    // What is the difference with x = var?;
    rt::Object* obj = rt::api::create_object(nullptr);

    state.addVar(x, obj);
  }

  void Interpreter::evalStackAlloc(verona::ir::StackAlloc& node)
  {
    assert(node.left.size() == 1);
    string x = node.left[0]->name;
    string tpe = node.type->name;
    assert(!state.containsVariable(x));
    assert(state.containsType(tpe));

    TypeObj* tpeObj = state.getTypeByName(tpe);

    // TODO Again figure that out;
    rt::Object* obj = rt::api::create_object(nullptr);
    state.addVar(x, obj);
  }

  void Interpreter::evalCall(verona::ir::Call& node)
  {
    // TODO requires to define a frame.
  }
  void Interpreter::evalTailcall(verona::ir::Tailcall& node) {}
  void Interpreter::evalRegion(verona::ir::Region& node) {}
  void Interpreter::evalCreate(verona::ir::Create& node) {}
  void Interpreter::evalBranch(verona::ir::Branch& node) {}
  void Interpreter::evalReturn(verona::ir::Return& node) {}
  void Interpreter::evalError(verona::ir::Err& node) {}
  void Interpreter::evalCatch(verona::ir::Catch& node) {}
  void Interpreter::evalAcquire(verona::ir::Acquire& node) {}
  void Interpreter::evalRelease(verona::ir::Release& node) {}
  void Interpreter::evalFulfill(verona::ir::Fulfill& node) {}
  void Interpreter::evalFreeze(verona::ir::Freeze& node) {}
  void Interpreter::evalMerge(verona::ir::Merge& node) {}

} // namespace interpreter
