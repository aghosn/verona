#pragma once

#include "ir.h"

/**
 * Put useless small visitors here.
 */
namespace interpreter
{
  // A simple printer for ir nodes;
  class SimplePrinter : public verona::ir::Visitor
  {
  public:
    void visit(verona::ir::NodeDef* node);
  }; 
} // namespace interpreter
