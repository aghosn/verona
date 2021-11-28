#pragma once

#include <string>
#include <vector>

namespace mlexer {

enum TokenKind {

  // Constants
  True,
  False,
  Identifier,

  // Keywords
  Iso,
  Mut,
  Imm,
  Paused,
  Stack,
  Store,
  Lookup,
  Typetest,
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
  Acquire,
  Release,
  Fulfill,

  // Builtin symbols
  Dot,
  Comma,
  LParen,
  RParen,
  Equals,
  SemiColon,

  // Special End symbol
  Eof,

};

struct Token {
  // TODO figure out what we want.
  std::string text;
  TokenKind kind;
};

using Line = std::vector<Token>;

// TODO(aghosn) make this iterable.
class Lexer {
public:
  int la;
  int pos;
  std::string file;
  std::vector<Line> lines;

  Lexer(std::string path);
  ~Lexer();

  Token next();
  bool isEmpty();
  bool hasNext();

  Token peek();
  void rewind();
  void reset();

private:
  void parseFile();
  Line parseLine(std::string line);
  Token parseWord(std::string word);
};

const char *tokenkindname(TokenKind t);

} // namespace mlexer
