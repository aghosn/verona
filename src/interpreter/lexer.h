#pragma once

#include <vector>
#include <string>

namespace mlexer {
  
  enum TokenKind {
    
    // Constants
    True,
    False,

    // Keywords
    Iso,
    Mut,
    Imm,
    Paused,
    Stack,
    Store,
    Var,
    Dup,
    Load,
    New,
    Call,
    Region,
    Create,
    Tailcall,
    Branch,
    Return,
    Error,
    Catch,
    acquire,
    Release,
    Fulfill,

    // Builtin symbols
    Dot,
    Comma,
    LParen,
    RParen,
    Equals,

  };

  struct Token {
    //TODO figure out what we want.
    TokenKind kind;
  };

  struct Line {
    std::vector<Token> tokens;
  };

  class Lexer {
    public:
    std::string file;
    std::vector<Line> lines;

    Lexer(std::string path);
    ~Lexer();

    private:
    void parseFile();
    Line parseLine(std::string line);
    Token parseWord(std::string word);

  };

} // namespace mlexer;
