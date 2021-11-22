// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT
#pragma once

#include "source.h"

namespace verona::parser
{
  enum class TokenKind
  {
    Invalid,

    // Builtin symbols
    Dot,
    Ellipsis,
    Comma,
    LParen,
    RParen,
    LSquare,
    RSquare,
    LBrace,
    RBrace,
    Semicolon,
    Colon,
    DoubleColon,
    FatArrow,
    Equals,

    // Constants
    EscapedString,
    UnescapedString,
    Character,
    Int,
    Float,
    Hex,
    Binary,
    Bool,

    // Keywords
    Class,
    Type,
    Try,
    Catch,
    Throw,
    Match,
    Var,
    Dup,
    Load,
    Store,
    Lookup,
    Typetest,
    New,
    Call,
    Region,
    Create,
    Tailcall,
    Branch,
    Return,
    Error,
    Acquire,
    Release,
    Fulfill,

    // Types
    Iso,
    Mut,
    Imm,
    Paused,
    Stack,
    Self,

    // Strategies
    GC,
    RC,
    Arena,


    // Symbols and identifiers
    Symbol,
    Ident,

    End
  };

  struct Token
  {
    TokenKind kind;
    Location location;
  };

  Token lex(Source& source, size_t& i);
}

