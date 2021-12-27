#pragma once
#include <map>
#include <utility>
#include <verona.h>

#include "ir.h"

using namespace std;
namespace rt = verona::rt;

namespace interpreter
{

  // TODO figure this out. 
  // For the moment just do it stupidly.
  // TODO Shoud probably have frames for names, e.g., a stack of maps.
  // For the moment leave that on the side.
  struct State {
    // Named live variables.
    // Map from name -> (object, ast definition);
    map<string, rt::Object*> store;
  };

} // namespace interpreter
