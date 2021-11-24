/**
 * This files is hosting my BS hacky implementations.
 * It exposes an API so that we can easily replace BS functions with proper
 * C++ implementations.
 */

#pragma once

#include <vector>
#include <string>

namespace mlexer::helpers {
  
  std::vector<std::string> split(std::string& str);

}


