/**
 * This files is hosting my BS hacky implementations.
 * It exposes an API so that we can easily replace BS functions with proper
 * C++ implementations.
 */

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace mlexer::helpers
{
  std::vector<std::pair<std::string, int>> split(std::string& str);

}
