#pragma once 

#include <iostream>

#include "ir.h"
#include "lexer.h"

namespace verona::ir {

  class Parser {
    public:
      mlexer::Lexer& lexer;

      Parser(mlexer::Lexer& lexer);
      ~Parser();

      bool parse();
      void parseEOL();
      Node<ID> parseIdentifier();
      Node<Expr> parseStatement();
      List<ID> parseListUntil(mlexer::TokenKind k);
      Node<Assign> parseRight(List<ID> left);
      std::pair<Node<ID>, List<ID>> parseApply();
      AllocStrategy parseStrategy();
      Node<TypeId> parseTypeId();

  };

 /* std::pair<bool, Ast> parse(
      const std::string& path,
      std::ostream& out = std::cerr
      );*/

} // namespace verona::ir
