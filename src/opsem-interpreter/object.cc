#include "object.h"

#include <sstream>

unsigned int objectid = 0;

namespace interpreter
{
  std::string nextObjectId()
  {
    std::stringstream ss;
    std::string s;
    ss << objectid++;
    ss >> s;
    return s;
  }

} // namespace interpreter
