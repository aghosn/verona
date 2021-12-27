#pragma once
#include <map>
#include <utility>
#include <verona.h>

#include "ir.h"

using namespace std;
namespace rt = verona::rt;

namespace interpreter
{

  //TODO fix once I know how types are represented.
  typedef rt::Object TypeObj;

  // TODO figure this out. 
  // For the moment just do it stupidly.
  // TODO Shoud probably have frames for names, e.g., a stack of maps.
  // For the moment leave that on the side.
  struct State {
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
