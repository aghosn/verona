#pragma once

#include <string>
#include <vector>

namespace mlexer
{
  enum TokenKind
  {

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
    Freeze,
    Merge,
    Function,
    Class,

    // Builtin symbols
    Dot,
    Comma,
    LParen,
    RParen,
    Equals,
    Colon,
    SemiColon,
    LBracket,
    RBracket,
    Pipe,
    And,

    // Special End symbol
    Eof,

  };

  struct Token
  {
    // TODO figure out what we want.
    std::string text;
    TokenKind kind;

    // Debugging information
    int la;
    int pos;
  };

  using Line = std::vector<Token>;

  // TODO(aghosn) make this iterable.
  class Lexer
  {
  public:
    int la;
    int pos;
    std::string file;
    std::vector<Line> lines;

    // For debug, keep the original content
    std::vector<std::string> content;

    Lexer(std::string path);
    ~Lexer();

    Token next();
    bool isEmpty();
    bool hasNext();

    Token peek();
    void rewind();
    void reset();
    void dump(int la, int col, int lines, bool error);

  private:
    void parseFile();
    Line parseLine(std::string line);
    Token parseWord(std::string word);
  };

  const char* tokenkindname(TokenKind t);

} // namespace mlexer
