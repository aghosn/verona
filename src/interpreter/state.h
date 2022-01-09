#pragma once
#include <map>
#include <vector>
#include <utility>
#include <verona.h>

#include "ir.h"
#include "type.h"
#include "utils.h" 
#include "object.h"

using namespace std;

using ObjectId = string;

namespace interpreter
{

  //TODO fix once I know how types are represented.
  typedef rt::Object TypeObj;

  // P ∈ Program ::= (Id → Function) × (TypeId → Type)
  struct Program {
    Map<Id, ir::Node<ir::Function>> functions;
    Map<TypeId, ir::Type> types;
  };

  // ϕ ∈ Frame ::= Region* × (Id → Value) × Id* × Expression*
  struct Frame {
    List<rt::Region*> regions; 
    Map<Id, Shared<ir::Value>> lookup;
    List<Id> rets;
    List<Shared<ir::Expr>> continuations;
    
    bool containsName(Id name) {
      if (!lookup.contains(name)) {
        return false;
      }
      auto value = lookup[name];
      return (value->kind() == ir::Kind::Object); 
    }

    Shared<ir::Value> frameLookup(Id name) {
      assert(lookup.contains(name));
      auto obj = lookup[name];
      assert(obj->kind() == ir::Kind::ObjectID);
      return obj;
    }
  };

  //σ ∈ State ::= Frame*
  //          × (ObjectId → Object)
  //          × (StorageLoc → Value)
  //          × (Region → Strategy)
  //          × Bool
  struct State {
    List<Shared<Frame>> frames;
    
    // TODO not sure why this is needed.
    // This really needs to be an ObjectID
    Map<ObjectId, Shared<Object>> objects;

    Map<ObjectId, Map<Id, Shared<ir::Value>>> fields;

    Map<rt::Region*, rt::RegionType> regions; 

    //TODO figure out they have both in the rules
    bool success; 
    bool except;

    Map<TypeId, Shared<Type>> types; 
    
    // Checks wheter a name is defined in the current scope.
    bool isDefinedInFrame(string name) {
      assert(frames.size() > 0);
      return frames.back()->containsName(name);
    }

    void addObject(ObjectId oid, Shared<Object> obj) {
      objects[oid] = obj;
    } 

    Shared<ir::Value> frameLookup(Id name) {
      assert(frames.size() > 0);
      return frames.back()->frameLookup(name);
    } 

    Shared<Object> getObjectByName(string name) {
      assert(frames.size() > 0);
      auto val = frames.back()->frameLookup(name);
      assert(val->kind() == ir::Kind::ObjectID && "Value is not an ObjectID");
      Shared<ir::ObjectID> objid = dynamic_pointer_cast<ir::ObjectID>(val);
      
      assert(objects.contains(objid->name) && "Object does not exist");
      
      return objects[objid->name];
    }

    Shared<ir::Value> getValueByName(string name) {
      assert(frames.size() > 0);
      return frames.back()->frameLookup(name);
    }

    void addInFrame(Id name, Shared<ir::Value> o) {
      assert(frames.size() > 0);
      //TODO should we check if the name already exists?
      frames.back()->lookup[name] = o; 
    }

    void removeFromFrame(Id name) {
      assert(frames.size() > 0);
      frames.back()->lookup.erase(name);
    }

    bool containsType(TypeId name) {
      return types.contains(name);
    }

    Shared<Type> getTypeByName(TypeId name) {
      return types[name];
    }

    bool isFunction(Id name) {
      // TODO Implement
      return true;
    }

    ir::Node<ir::Function> getFunction(Id name) {
      return nullptr;
    }
  };

} // namespace interpreter
