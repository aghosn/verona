#include "parser.h"

namespace verona::ir {

  enum Result {
    Skip,
    Sucess,
    Error,
  };

  struct Parse {


  std::pair<bool, Ast> parse(const std::string& path, std::ostream& out) {
    return std::pair<bool, Ast>(false, Ast());
  }
  };

} // namespace verona::ir
