#include "parser.h"

#include "lexer.h"

#include <cassert>

using namespace mlexer;
using namespace std;

namespace verona::ir
{
  enum Result
  {
    Skip,
    Sucess,
    Error,
  };

  Parser::Parser(mlexer::Lexer& lexer) : lexer(lexer) {}

  Parser::~Parser(){};

  // Parses an expression
  Node<Expr> Parser::parseStatement()
  {
    // Can only be an identifier or a keyword
    Token t = lexer.next();
    switch (t.kind)
    {
      // It is a name
      case TokenKind::Identifier:
      {
        auto left = List<ID>();
        auto id = make_shared<ID>();
        id->name = t.text;
        left.push_back(id);

        // We have a list
        if (lexer.peek().kind == TokenKind::Comma)
        {
          lexer.rewind();
          left = parseListUntil(TokenKind::Equals);
        }
        auto res = parseRight(left);
        parseEOL();
        return res;
      }
        assert(0);
      case TokenKind::Tailcall:
      {
        auto tailcall = make_shared<Tailcall>();
        auto call = parseApply();
        tailcall->function = call.first;
        tailcall->args = call.second;
        parseEOL();
        return tailcall;
      }
        assert(0);
      case TokenKind::Branch:
      {
        auto branch = make_shared<Branch>();
        branch->condition = parseIdentifier<ID>();

        // First branch
        branch->branch1 = make_shared<Apply>();
        auto b1 = parseApply();
        branch->branch1->function = b1.first;
        branch->branch1->args = b1.second;

        // Second branch
        branch->branch2 = make_shared<Apply>();
        auto b2 = parseApply();
        branch->branch2->function = b2.first;
        branch->branch2->args = b2.second;
        parseEOL();
        return branch;
      }
        assert(0);
      case TokenKind::Return:
      {
        auto ret = make_shared<Return>();
        ret->returns = parseListUntil(TokenKind::SemiColon);
        parseEOL();
        return ret;
      }
        assert(0);
      case TokenKind::Error:
      {
        parseEOL();
        // TODO(aghosn) fix
        auto err = make_shared<Err>();
        return err;
      }
        assert(0);
      case TokenKind::Acquire:
      {
        auto acquire = make_shared<Acquire>();
        acquire->target = parseIdentifier<ID>();
        parseEOL();
        return acquire;
      }
        assert(0);
      case TokenKind::Release:
      {
        auto release = make_shared<Release>();
        // TODO handle the value case
        release->target = parseIdentifier<ID>();
        parseEOL();
        return release;
      }
        assert(0);
      case TokenKind::Fulfill:
      {
        auto fulfill = make_shared<Fulfill>();
        fulfill->target = parseIdentifier<ID>();
        parseEOL();
        return fulfill;
      }
        assert(0);
      case TokenKind::Function:
      {
        return parseFunction();
      }
        assert(0);
      case TokenKind::Class:
      {
        auto classdecl = make_shared<Class>();
        classdecl->id = parseIdentifier<ClassID>();
        classdecl->members = parseMembers();
        dropExpected(TokenKind::RBracket);
        parseEOL();
        return classdecl;
      }
        assert(0);
      case TokenKind::Call:
      {
        // A call without return values
        auto call = make_shared<Call>();
        auto apply = parseApply();
        call->function = apply.first;
        call->args = apply.second;
        parseEOL();
        return call;
      }
        assert(0);
      default:
        // No match
        std::cerr << "Unrecognized kind " << t.kind << std::endl;
        assert(0);
    }
    return nullptr;
  }

  Node<Function> Parser::parseFunction()
  {
    auto function = make_shared<Function>();
    auto apply = parseApply();
    function->function = apply.first;
    function->args = apply.second;
    function->exprs = parseBlock();
    return function;
  }

  List<ID> Parser::parseListUntil(TokenKind k)
  {
    List<ID> result;
    bool id = true;
    while (lexer.peek().kind != k)
    {
      Token t = lexer.peek();
      assert(t.kind == TokenKind::Identifier || t.kind == TokenKind::Comma);
      if (t.kind == TokenKind::Comma)
      {
        assert(result.size() != 0);
        assert(id == false);
        dropExpected(TokenKind::Comma);
        id = true;
        continue;
      }
      id = false;
      result.push_back(parseIdentifier<ID>());
    }
    return result;
  }

  template<typename T>
  Node<T> Parser::parseIdentifier()
  {
    assert(lexer.hasNext());
    Token t = lexer.peek();
    assert(t.kind == TokenKind::Identifier);
    lexer.next();
    auto id = make_shared<T>();
    id->name = t.text;
    return id;
  }

  Node<StorageLoc> Parser::parseStorageLoc()
  {
    assert(lexer.hasNext());
    auto oid = parseIdentifier<ID>();

    // parse the dot
    assert(lexer.hasNext());
    Token t = lexer.peek();
    assert(t.kind == TokenKind::Dot);
    lexer.next();
    auto id = parseIdentifier<ID>();
    auto storage = make_shared<StorageLoc>();
    storage->objectid = oid;
    storage->id = id;
    return storage;
  }

  void Parser::parseEOL()
  {
    assert(lexer.hasNext());
    Token t = lexer.peek();
    if (!(t.kind == TokenKind::SemiColon))
    {
      assert(0);
    }
    assert(t.kind == TokenKind::SemiColon);
    lexer.next();
  }

  std::pair<Node<ID>, List<ID>> Parser::parseApply()
  {
    auto function = parseIdentifier<ID>();
    // TODO something for parentheses less ugly then that.
    auto t = lexer.next();
    assert(t.kind == TokenKind::LParen);
    auto args = parseListUntil(TokenKind::RParen);
    dropExpected(TokenKind::RParen);
    return std::pair<Node<ID>, List<ID>>(function, args);
  }

  void Parser::dropExpected(TokenKind k)
  {
    assert(lexer.peek().kind == k);
    lexer.next();
  }

  AllocStrategy Parser::parseStrategy()
  {
    Token t = lexer.next();
    assert(t.kind == TokenKind::Identifier);
    if (t.text == "GC")
    {
      return AllocStrategy::Trace; // GC;
    }

    if (t.text == "RC")
    {
      return AllocStrategy::Rc;
    }

    if (t.text == "Arena")
    {
      return AllocStrategy::Arena;
    }

    std::cerr << "Wrong alloc strategy :'" << t.text << "'" << std::endl;
    assert(0);
    return AllocStrategy::Trace;
  }

  Node<Assign> Parser::parseRight(List<ID> v)
  {
    dropExpected(TokenKind::Equals);
    Token t = lexer.next();
    switch (t.kind)
    {
      case TokenKind::Catch:
      {
        auto ctch = make_shared<Catch>();
        ctch->left = v;
        return ctch;
      }
        assert(0);
      case TokenKind::Var:
      {
        auto var = make_shared<Var>();
        var->left = v;
        return var;
      }
        assert(0);
      case TokenKind::Dup:
      {
        auto dup = make_shared<Dup>();
        dup->y = parseIdentifier<ID>();
        dup->left = v;
        return dup;
      }
        assert(0);
      case TokenKind::Load:
      {
        auto load = make_shared<Load>();
        load->left = v;
        load->source = parseIdentifier<ID>();
        return load;
      }
        assert(0);
      case TokenKind::Store:
      {
        auto store = make_shared<Store>();
        store->left = v;
        store->y = parseStorageLoc();
        store->z = parseIdentifier<ID>();
        return store;
      }
        assert(0);
      case TokenKind::Lookup:
      {
        auto lookup = make_shared<Lookup>();
        lookup->left = v;
        lookup->y = parseIdentifier<ID>();
        lookup->z = parseIdentifier<ID>();
        return lookup;
      }
        assert(0);
      case TokenKind::Typetest:
      {
        auto tpetest = make_shared<Typetest>();
        tpetest->left = v;
        tpetest->y = parseIdentifier<ID>();
        tpetest->type = parseTypeId();
        return tpetest;
      }
        assert(0);
      case TokenKind::New:
      {
        auto nnew = make_shared<NewAlloc>();
        nnew->left = v;
        nnew->type = parseTypeId();
        return nnew;
      }
        assert(0);
      case TokenKind::Stack:
      {
        auto nstack = make_shared<StackAlloc>();
        nstack->left = v;
        nstack->type = parseTypeId();
        return nstack;
      }
        assert(0);
      case TokenKind::Call:
      {
        auto call = make_shared<Call>();
        auto apply = parseApply();
        call->left = v;
        call->function = apply.first;
        call->args = apply.second;
        return call;
      }
        assert(0);
      case TokenKind::Region:
      {
        auto region = make_shared<Region>();
        auto apply = parseApply();
        region->left = v;
        assert(apply.second.size() >= 1);
        region->function = apply.first;
        region->args = apply.second;
        return region;
      }
        assert(0);
      case TokenKind::Create:
      {
        auto create = make_shared<Create>();
        create->strategy = parseStrategy();
        auto apply = parseApply();
        create->left = v;
        create->function = apply.first;
        create->args = apply.second;
        return create;
      }
        assert(0);
      case TokenKind::Freeze:
      {
        auto freeze = make_shared<Freeze>();
        freeze->left = v;
        freeze->target = parseIdentifier<ID>();
        return freeze;
      }
        assert(0);
      case TokenKind::Merge:
      {
        auto merge = make_shared<Merge>();
        merge->left = v;
        merge->target = parseIdentifier<ID>();
        return merge;
      }
        assert(0);
      default:
        std::cerr << "Could not parse right expression: '" << t.kind
                  << std::endl;
        assert(0);
    }

    assert(0);
    return nullptr;
  }

  Node<TypeId> Parser::parseTypeId()
  {
    assert(lexer.hasNext());
    Token t = lexer.peek();
    assert(t.kind == TokenKind::Identifier);
    lexer.next();
    auto id = make_shared<TypeId>();
    id->name = t.text;
    return id;
  }

  List<Expr> Parser::parseBlock()
  {
    List<Expr> body;
    dropExpected(TokenKind::LBracket);
    parseEOL();
    while (lexer.peek().kind != TokenKind::RBracket)
    {
      body.push_back(parseStatement());
    }
    assert(lexer.peek().kind == TokenKind::RBracket);
    dropExpected(TokenKind::RBracket);
    parseEOL();
    return body;
  }

  Map<Id, Member> Parser::parseMembers()
  {
    Map<Id, Member> members;
    dropExpected(TokenKind::LBracket);
    parseEOL();
    while (lexer.peek().kind != TokenKind::RBracket)
    {
      auto t = lexer.peek();
      switch (t.kind)
      {
        case TokenKind::Identifier:
        {
          auto field = parseField();
          members[field->id->name] = field;
          break;
        }
          assert(0);
        case TokenKind::Function:
        {
          dropExpected(TokenKind::Function);
          auto function = parseFunction();
          members[function->function->name] = function;
          break;
        }
          assert(0);
        default:
          assert(0 && "Invalid token in members");
      }
    }
    return members;
  }

  // TODO correct this, it should be {f: T}
  Node<Field> Parser::parseField()
  {
    auto field = make_shared<Field>();
    field->id = parseIdentifier<ID>();
    dropExpected(TokenKind::Colon);
    field->type = parseTypeRef();
    parseEOL();
    return field;
  }

  Node<TypeRef> Parser::parseTypeRef()
  {
    auto t = lexer.peek();
    Node<TypeRef> result = nullptr;
    switch (t.kind)
    {
      case TokenKind::Iso:
        result = make_shared<Iso>();
        break;
      case TokenKind::Mut:
        result = make_shared<Mut>();
        break;
      case TokenKind::Imm:
        result = make_shared<Imm>();
        break;
      case TokenKind::Paused:
        result = make_shared<Paused>();
        break;
      case TokenKind::Stack:
        result = make_shared<Stack>();
        break;
      case TokenKind::Identifier:
      {
        result = parseIdentifier<TypeId>();
        assert(result != nullptr);
        break;
      }
      case TokenKind::LParen:
        result = parseTypeOp();
        break;
      case TokenKind::RParen:
        assert(0 && "We hit that");
        break;
      // The left side has been parsed.
      case TokenKind::Comma:
      {
        auto tuple = make_shared<TupleType>();
        dropExpected(TokenKind::Comma);
        tuple->right = parseTypeRef();
        result = tuple;
        break;
      }
        assert(0);
      case TokenKind::Pipe:
      {
        auto pipe = make_shared<UnionType>();
        dropExpected(TokenKind::Pipe);
        pipe->right = parseTypeRef();
        result = pipe;
        break;
      }
        assert(0);
      case TokenKind::And:
      {
        auto uni = make_shared<IsectType>();
        dropExpected(TokenKind::And);
        uni->right = parseTypeRef();
        result = uni;
        break;
      }
        assert(0);
      case TokenKind::Store:
      {
        auto store = make_shared<StoreType>();
        dropExpected(TokenKind::Store);
        store->type = parseTypeRef();
        result = store;
        break;
      }
        assert(0);
      default:
        assert(0 && "Unknown type construct");
    }
    assert(result != nullptr);
    return result;
  }

  Node<TypeRef> Parser::parseTypeOp()
  {
    auto t = lexer.peek();
    assert(t.kind == TokenKind::LParen);
    dropExpected(TokenKind::LParen);
    Node<TypeRef> left = nullptr;
    while (lexer.peek().kind != TokenKind::RParen)
    {
      auto type = parseTypeRef();
      assert(type != nullptr);

      if (left == nullptr)
      {
        left = type;
        continue;
      }

      assert(left != nullptr);
      switch (type->kind())
      {
        case Kind::TupleType:
        case Kind::UnionType:
        case Kind::IsectType:
        {
          auto typeop = dynamic_pointer_cast<TypeOp>(type);
          assert(typeop->left == nullptr);
          typeop->left = left;
          left = type;
          break;
        }
          assert(0);
        default:
          assert(0 && "This should not happen");
      }
    }
    assert(lexer.peek().kind == TokenKind::RParen);
    dropExpected(TokenKind::RParen);
    return left;
  }

  bool Parser::parse()
  {
    while (lexer.hasNext())
    {
      auto statement = parseStatement();
      if (statement->kind() == Kind::Function)
      {
        auto func = dynamic_pointer_cast<Function>(statement);
        functions.push_back(func);
      }
      else if (statement->kind() == Kind::Class)
      {
        auto classdecl = dynamic_pointer_cast<Class>(statement);
        classes.push_back(classdecl);
      }
      else
      {
        // TODO not even sure this should be possible.
        program.push_back(statement);
      }
    }

    return true;
  }

} // namespace verona::ir
