#include "lexer.h"

#include "helpers.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <stdlib.h>

// Functions to parse strings.
using namespace mlexer::helpers;

namespace mlexer
{
  // Replace thar thing with a map directly;
  std::map<std::string, TokenKind> keywords = {
    {std::string("true"), TokenKind::True},
    {std::string("false"), TokenKind::False},
    {std::string("iso"), TokenKind::Iso},
    {std::string("mut"), TokenKind::Mut},
    {std::string("imm"), TokenKind::Imm},
    {std::string("paused"), TokenKind::Paused},
    {std::string("stack"), TokenKind::Stack},
    {std::string("store"), TokenKind::Store},
    {std::string("lookup"), TokenKind::Lookup},
    {std::string("typetest"), TokenKind::Typetest},
    {std::string("var"), TokenKind::Var},
    {std::string("dup"), TokenKind::Dup},
    {std::string("load"), TokenKind::Load},
    {std::string("new"), TokenKind::New},
    {std::string("stack"), TokenKind::Stack},
    {std::string("call"), TokenKind::Call},
    {std::string("region"), TokenKind::Region},
    {std::string("create"), TokenKind::Create},
    {std::string("tailcall"), TokenKind::Tailcall},
    {std::string("branch"), TokenKind::Branch},
    {
      std::string("return"),
      TokenKind::Return,
    },
    {std::string("error"), TokenKind::Error},
    {std::string("catch"), TokenKind::Catch},
    {
      std::string("acquire"),
      TokenKind::Acquire,
    },
    {std::string("release"), TokenKind::Release},
    {std::string("fulfill"), TokenKind::Fulfill},
    {std::string("freeze"), TokenKind::Freeze},
    {std::string("merge"), TokenKind::Merge},
    {std::string("fn"), TokenKind::Function},
    {std::string("class"), TokenKind::Class},
  };

  std::map<std::string, TokenKind> builtins = {
    {std::string("."), Dot},
    {std::string(","), Comma},
    {std::string("("), LParen},
    {std::string(")"), RParen},
    {std::string("="), Equals},
    {std::string(":"), Colon},
    {std::string(";"), SemiColon},
    {std::string("{"), LBracket},
    {std::string("}"), RBracket},
    {std::string("|"), Pipe},
    {std::string("&"), And},
  };

  Lexer::Lexer(std::string path) : file(path), la(0), pos(0)
  {
    // Read the entire file.
    this->parseFile();
  }

  Lexer::~Lexer() {}

  void Lexer::parseFile()
  {
    std::ifstream file(this->file);
    if (file.fail())
    {
      std::cerr << "Could not open file " << this->file << std::endl;
      exit(EXIT_FAILURE);
    }

    std::string str;
    while (std::getline(file, str))
    {
      // Skip empty lines
      if (!std::all_of(str.begin(), str.end(), isspace))
      {
        this->lines.push_back(this->parseLine(str));
      }
    }
  }

  // Helper function to split string according to whitespaces.

  Line Lexer::parseLine(std::string line)
  {
    Line result;
    auto words = split(line);
    for (auto w : words)
    {
      result.push_back(parseWord(w));
    }
    Token t = {";", TokenKind::SemiColon};
    result.push_back(t);
    return result;
  }

  Token Lexer::parseWord(std::string word)
  {
    Token result;
    result.text = word;
    // This is a keyword
    if (keywords.contains(word))
    {
      result.kind = keywords[word];
      return result;
    }

    // Valid operations and symbols;
    if (builtins.contains(word))
    {
      result.kind = builtins[word];
      return result;
    }

    // Identifier
    result.kind = Identifier;
    return result;
  }

  bool Lexer::isEmpty()
  {
    return lines.size() == 0;
  }

  bool Lexer::hasNext()
  {
    return (!this->isEmpty() && la != -1);
  }

  // TODO(aghosn) improve that thing
  Token Lexer::next()
  {
    Token t;
    if (!hasNext() || la >= lines.size())
    {
      goto Empty;
    }

    assert(la >= 0 && la < lines.size());
    assert(pos >= 0 && pos < lines[la].size());
    t = lines[la][pos];
    pos++;

    // Cleanup: handle the wrapping
    if (pos >= lines[la].size())
    {
      la++;
      pos = 0;
    }

    // We are empty
    if (la >= lines.size())
    {
      la = -1;
    }

    return t;

  Empty:
    la = -1;
    pos = 0;
    t = Token{"EOF", TokenKind::Eof};
    return t;
  }

  Token Lexer::peek()
  {
    if (!hasNext())
    {
      return Token{"EOF", TokenKind::Eof};
    }
    assert(la >= 0 && la <= lines.size());

    return lines[la][pos];
  }

  void Lexer::rewind()
  {
    la = (la < 0) ? lines.size() : la;
    if (la == 0 && pos == 0)
    {
      return;
    }
    pos--;
    if (pos < 0)
    {
      la--;
      assert(la >= 0 && la < lines.size());
      pos = lines.size() - 1;
    }
  }

  void Lexer::reset()
  {
    la = 0;
    pos = 0;
  }

  const char* tokenkindname(TokenKind t)
  {
    switch (t)
    {
      case TokenKind::True:
        return "true";
      case TokenKind::False:
        return "false";
      case TokenKind::Identifier:
        return "identifier";
      case TokenKind::Iso:
        return "iso";
      case TokenKind::Mut:
        return "mut";
      case TokenKind::Imm:
        return "imm";
      case TokenKind::Paused:
        return "paused";
      case TokenKind::Stack:
        return "stack";
      case TokenKind::Store:
        return "store";
      case TokenKind::Lookup:
        return "lookup";
      case TokenKind::Typetest:
        return "typetest";
      case TokenKind::Var:
        return "var";
      case TokenKind::Dup:
        return "dup";
      case TokenKind::Load:
        return "load";
      case TokenKind::New:
        return "new";
      case TokenKind::Call:
        return "call";
      case TokenKind::Region:
        return "region";
      case TokenKind::Create:
        return "create";
      case TokenKind::Tailcall:
        return "tailcall";
      case TokenKind::Branch:
        return "branch";
      case TokenKind::Return:
        return "return";
      case TokenKind::Error:
        return "error";
      case TokenKind::Catch:
        return "catch";
      case TokenKind::Acquire:
        return "acquire";
      case TokenKind::Release:
        return "release";
      case TokenKind::Fulfill:
        return "fulfill";
      case TokenKind::Freeze:
        return "freeze";
      case TokenKind::Merge:
        return "merge";
      case TokenKind::Function:
        return "fn";
      case TokenKind::Class:
        return "class";
      case TokenKind::Dot:
        return ".";
      case TokenKind::Comma:
        return ",";
      case TokenKind::LParen:
        return "(";
      case TokenKind::RParen:
        return ")";
      case TokenKind::Equals:
        return "=";
      case TokenKind::Colon:
        return ":";
      case TokenKind::SemiColon:
        return ";\n";
      case TokenKind::LBracket:
        return "{";
      case TokenKind::RBracket:
        return "}";
      case TokenKind::Pipe:
        return "|";
      case TokenKind::And:
        return "&";
      default:
        assert(0);
    }
    return nullptr;
  }

} // namespace mlexer
