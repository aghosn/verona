#pragma once

#include <vector>

#include "rules.h"

namespace interpreter {


class Interpreter {
  public:
  std::vector<IRule*> rules; 

  Interpreter();

  std::vector<IRule*> match(Ast t);

};

} // namespace verona::ir
