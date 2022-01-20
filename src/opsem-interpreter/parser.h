#pragma once

#include "ir.h"
#include "lexer.h"

#include <iostream>

namespace verona::ir
{
  class Parser
  {
  public:
    AstPath program;
    List<Function> functions;
    List<Class> classes;
    mlexer::Lexer& lexer;

    Parser(mlexer::Lexer& lexer);
    ~Parser();

    // Parses the entire program.
    bool parse();

    // Helpful functions called within the parser.
    void parseEOL();
    template<typename T>
    Node<T> parseIdentifier();
    Node<Expr> parseStatement();
    Node<Function> parseFunction();
    List<Expr> parseBlock();
    Map<Id, Member> parseMembers();
    Node<Field> parseField();
    Node<TypeRef> parseTypeRef();
    Node<TypeRef> parseTypeOp();
    List<ID> parseListUntil(mlexer::TokenKind k);
    Node<Assign> parseRight(List<ID> left);
    std::pair<Node<ID>, List<ID>> parseApply();
    AllocStrategy parseStrategy();
    Node<TypeId> parseTypeId();
    void dropExpected(mlexer::TokenKind k);
    Node<StorageLoc> parseStorageLoc();
  };

} // namespace verona::ir
