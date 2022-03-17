#pragma once
#include "ir.h"
#include "object.h"
#include "type.h"
#include "utils.h"
#include "interoperability.h"
#include "error.h"

#include <cassert>
#include <map>
#include <utility>
#include <memory>
#include <vector>
#include <verona.h>
#include <iostream>

using ObjectId = std::string;

namespace interpreter
{
  // P ∈ Program ::= (Id → Function) × (TypeId → Type)
  struct Program
  {
    Map<Id, ir::Node<ir::Function>> functions;
    Map<TypeId, Shared<Type>> types;

    // sandboxed code, for the moment only one
    interop::SandboxConfig* sandbox;
  };

  // ϕ ∈ Frame ::= Region* × (Id → Value) × Id* × Expression*
  struct Frame
  {
    List<Region*> regions;
    Map<Id, Shared<ir::Value>> lookup;
    List<Id> rets;
    List<Shared<ir::Expr>> continuations;

    bool containsName(Id name)
    {
      return (lookup.find(name) != lookup.end());
    }

    Shared<ir::Value> frameLookup(Id name)
    {
      assert(lookup.find(name) != lookup.end());
      auto value = lookup[name];
      return value;
    }
  };

  // ExecState represents the current program pointer.
  // We use an int so that we can encode special values as negative numbers.
  struct ExecState
  {
    ir::List<ir::Expr> exprs;
    int64_t offset;

    ir::List<ir::Expr> getContinuation()
    {
      assert(offset < exprs.size() - 1);
      ir::List<ir::Expr> cont;
      for (int i = offset + 1; i < exprs.size(); i++)
      {
        cont.push_back(exprs[i]);
      }
      return cont;
    }
  };

  //σ ∈ State ::= Frame*
  //          × (ObjectId → Object)
  //          × (StorageLoc → Value)
  //          × (Region → Strategy)
  //          × Bool
  struct State
  {
    List<Shared<Frame>> frames;

    // TODO not sure why this is needed.
    // This really needs to be an ObjectID
    Map<ObjectId, Object*> objects;

    Map<ObjectId, Map<Id, Shared<ir::Value>>> fields;

    Map<Region*, ir::AllocStrategy> regions;

    // TODO figure out they have both in the rules
    bool success;
    bool except;

    // Execution state
    ExecState exec_state;

    // Program
    Program program;

    // Constructor
    void init(ir::List<ir::Class> classes, ir::List<ir::Function> functions)
    {
      for (auto c : classes)
      {
        program.types[c->id->name] = c;
      }
      // Create the artifical type Bool
      auto boolClass = std::make_shared<ir::Class>();
      boolClass->id = std::make_shared<ir::ClassID>();
      boolClass->id->name = "Bool";
      program.types["Bool"] = boolClass;

      for (auto f : functions)
      {
        program.functions[f->function->name] = f;
      }

      // TODO figure that out, should keep only one
      // and we should know when we are done
      except = false;
      success = true;
    }

    // Checks wheter a name is defined in the current scope.
    bool isDefinedInFrame(std::string name)
    {
      assert(frames.size() > 0);
      return frames.back()->containsName(name);
    }

    void addObject(ObjectId oid, Object* obj)
    {
      objects[oid] = obj;
    }

    Shared<ir::Value> frameLookup(Id name)
    {
      assert(frames.size() > 0);
      return frames.back()->frameLookup(name);
    }

    Object* getObjectByName(std::string name)
    {
      assert(frames.size() > 0);
      auto val = frames.back()->frameLookup(name);
      assert(val->kind() == ir::Kind::ObjectID && "Value is not an ObjectID");
      Shared<ir::ObjectID> objid = std::dynamic_pointer_cast<ir::ObjectID>(val);

      assert((objects.find(objid->name) != objects.end()) && "Object does not exist");

      return objects[objid->name];
    }

    Shared<ir::Value> getValueByName(std::string name)
    {
      assert(frames.size() > 0);
      return frames.back()->frameLookup(name);
    }

    void addInFrame(Id name, Shared<ir::Value> o)
    {
      assert(frames.size() > 0);
      // TODO should we check if the name already exists?
      frames.back()->lookup[name] = o;
    }

    void removeFromFrame(Id name)
    {
      assert(frames.size() > 0);
      frames.back()->lookup.erase(name);
    }

    bool containsType(TypeId name)
    {
      return (program.types.find(name) != program.types.end());
    }

    Shared<Type> getTypeByName(TypeId name)
    {
      return program.types[name];
    }

    bool isFunction(Id name)
    {
      return (program.functions.find(name) != program.functions.end());
    }

    ir::Node<ir::Function> getFunction(Id name)
    {
      assert(isFunction(name) && "Function is not defined");
      return program.functions[name];
    }

    Object* newObject(void)
    {
      CHECK(frames.size() > 0, E_WRONG_NB_AT_LEAST(0, frames.size()));
      auto frame = frames.back();

      // Get the current region
      CHECK(frame->regions.size() > 0, E_WRONG_NB_AT_LEAST(0, frame->regions.size()));
      auto region = frame->regions.back();
      return region->alloc();
    }
  };

} // namespace interpreter
