#pragma once
#include "ir.h"
#include "object.h"
#include "type.h"
#include "utils.h"

#include <cassert>
#include <map>
#include <utility>
#include <vector>
#include <verona.h>

using namespace std;

using ObjectId = string;

namespace interpreter
{
  // TODO fix once I know how types are represented.
  typedef rt::Object TypeObj;

  // P ∈ Program ::= (Id → Function) × (TypeId → Type)
  struct Program
  {
    Map<Id, ir::Node<ir::Function>> functions;
    Map<TypeId, Shared<Type>> types;
  };

  // ϕ ∈ Frame ::= Region* × (Id → Value) × Id* × Expression*
  struct Frame
  {
    List<rt::Region*> regions;
    Map<Id, Shared<ir::Value>> lookup;
    List<Id> rets;
    List<Shared<ir::Expr>> continuations;

    bool containsName(Id name)
    {
      if (!lookup.contains(name))
      {
        return false;
      }
      auto value = lookup[name];
      return (value->kind() == ir::Kind::Object);
    }

    Shared<ir::Value> frameLookup(Id name)
    {
      assert(lookup.contains(name));
      auto obj = lookup[name];
      assert(obj->kind() == ir::Kind::ObjectID);
      return obj;
    }
  };

  struct ExecState
  {
    ir::List<ir::Expr> exprs;
    uint32_t offset;

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
    Map<ObjectId, Shared<Object>> objects;

    Map<ObjectId, Map<Id, Shared<ir::Value>>> fields;

    Map<rt::Region*, rt::RegionType> regions;

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
      for (auto f : functions)
      {
        program.functions[f->function->name] = f;
      }
    }

    // Checks wheter a name is defined in the current scope.
    bool isDefinedInFrame(string name)
    {
      assert(frames.size() > 0);
      return frames.back()->containsName(name);
    }

    void addObject(ObjectId oid, Shared<Object> obj)
    {
      objects[oid] = obj;
    }

    Shared<ir::Value> frameLookup(Id name)
    {
      assert(frames.size() > 0);
      return frames.back()->frameLookup(name);
    }

    Shared<Object> getObjectByName(string name)
    {
      assert(frames.size() > 0);
      auto val = frames.back()->frameLookup(name);
      assert(val->kind() == ir::Kind::ObjectID && "Value is not an ObjectID");
      Shared<ir::ObjectID> objid = dynamic_pointer_cast<ir::ObjectID>(val);

      assert(objects.contains(objid->name) && "Object does not exist");

      return objects[objid->name];
    }

    Shared<ir::Value> getValueByName(string name)
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
      return program.types.contains(name);
    }

    Shared<Type> getTypeByName(TypeId name)
    {
      return program.types[name];
    }

    bool isFunction(Id name)
    {
      return program.functions.contains(name);
    }

    ir::Node<ir::Function> getFunction(Id name)
    {
      assert(isFunction(name) && "Function is not defined");
      return program.functions[name];
    }
  };

} // namespace interpreter
