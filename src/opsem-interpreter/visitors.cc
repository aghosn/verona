#include "visitors.h"

#include "notation.h"
#include "state.h"
#include "utils.h"

#include <cassert>
#include <iostream>
#include <verona.h>

namespace interpreter
{
  void SimplePrinter::visit(ir::NodeDef* node)
  {
    std::cout << "The type " << verona::ir::kindname(node->kind()) << std::endl;
  }
} // namespace interpreter
