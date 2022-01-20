#include "state.h"
#include "notation.h"
#include "visitors.h"
#include "utils.h"

#include <cassert>
#include <iostream>
#include <verona.h>

namespace ir = verona::ir;
namespace rt = verona::rt;

namespace interpreter
{
  void SimplePrinter::visit(ir::NodeDef* node)
  {
    std::cout << "The type " << verona::ir::kindname(node->kind()) << std::endl;
  }
} // namespace interpreter
