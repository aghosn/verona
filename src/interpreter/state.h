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
    Map<Id, ir::Function*> functions;
    Map<TypeId, ir::Type> types;
  };

  // ϕ ∈ Frame ::= Region* × (Id → Value) × Id* × Expression*
  struct Frame {
    List<rt::Region*> regions; 
    Map<Id, Shared<ir::Value>> lookup;
    List<Id> rets;
    List<Shared<ir::Expr>> continuations;
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

    Map<Shared<StorageLoc>, Shared<ir::Value>> fields;

    Map<rt::Region*, rt::RegionType> regions; 

    bool except; 
    
    // Named live variables.
    // Map from name -> (object, ast definition);
    map<string, rt::Object*> store;

    //TODO figure out how they represent types.
    map<string, rt::Object*> types;

    // Checks wheter a name is defined in the current scope.
    // TODO handle multi scope/frames.
    bool containsVariable(string name) {
      return store.contains(name);
    }

    rt::Object* getVarByName(string name) {
      return store[name];
    }

    void addVar(string name, rt::Object* o) {
      store[name] = o;
    }

    bool containsType(string name) {
      return types.contains(name);
    }

    TypeObj* getTypeByName(string name) {
      return types[name];
    }
  };

} // namespace interpreter
