#pragma once 

#include <iostream>

#include "ir.h"

namespace verona::ir {

  std::pair<bool, Ast> parse(
      const std::string& path,
      std::ostream& out = std::cerr
      );

} // namespace verona::ir
