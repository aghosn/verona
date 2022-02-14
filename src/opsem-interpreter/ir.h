#pragma once

#include <map>
#include <memory>
#include <vector>
#include <verona.h>
#include "lexer.h"

namespace rt = verona::rt;

namespace verona::ir
{
  enum class Kind
  {
    Invalid,
    //Identifier,
    ID,
    TypeID,
    ObjectID,
    FunctionID,
    StorageLoc,
    Var,
    Dup,
    Load,
    Store,
    Lookup,
    Typetest,
    NewAlloc,
    StackAlloc,
    Call,
    Tailcall,
    Region,
    Create,
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

    // Types
    Iso,
    Mut,
    Imm,
    Paused,
    Stack,
    UnionType,
    IsectType,
    TupleType,
    StoreMod,
    Class,

    // Members
    Field,

    // Constant
    True,
    False,
    Undef,

    // Values
    Object,

    End,

  };

  const char* kindname(Kind k);

  struct NodeDef;

  // TODO(aghosn) figure out if we need that or not;
  template<typename T>
  using Node = std::shared_ptr<T>;

  template<typename T>
  using List = std::vector<Node<T>>;

  template<typename T, typename Y>
  using Map = std::map<T, Node<Y>>;

  using Ast = Node<NodeDef>;
  using AstPath = List<NodeDef>;
  using Id = std::string;

  /**
   * A visitor for Nodes
   * */
  class Visitor
  {
  public:
    virtual void visit(NodeDef* node) = 0;
  };

  /**
   * Everything is a NodeDef.
   *
   * @param location
   *
   * @method kind: returns the kind of this particular node.
   */
  struct NodeDef
  {
    mlexer::Token tok;

    // virtual ~NodeDef() = default;
    virtual Kind kind() = 0;

    template<typename T>
    T& as()
    {
      return static_cast<T&>(*this);
    }

    void accept(Visitor* visitor)
    {
      visitor->visit(this);
    }
  };

  /**
   * Expressions.
   */

  struct Expr : NodeDef
  {};

  // TODO(aghosn) avoid double inheritance from the same class.
  // so avoid values being express. Let them be just values;
  // Try to remove identifier maybe as well.

  /**
   * Forward declarations.
   */
  // Forward definition
  struct Value : NodeDef
  {};

  /**
   * Constants
   */

  struct Constant : Value
  {};

  /**
   * Identifier
   *
   * @comment: I put it in a struct for the moment in case I need more than just
   * a name. Maybe replace with a constant or make it extend unescaped string?
   */
  struct Identifier /*: Expr*/
  {
    std::string name;

    /*Kind kind() override
    {
      return Kind::Identifier;
    }*/
  };

  struct Type
  {
    virtual bool isStorable()
    {
      return false;
    }
  };
  struct TypeRef : NodeDef, Type
  {};

  struct TypeId : Identifier, TypeRef
  {
    Kind kind() override
    {
      return Kind::TypeID;
    }
  };

  // Member ::= Function | Id
  struct Member : NodeDef/*: Expr*/
  {};

  // TODO(aghosn) quick way to change if I fucked up.
  // Let's take a litteral approach to the problem.
  struct ID : Identifier, Member
  {
    Kind kind() override
    {
      return Kind::ID;
    }
  };

  // x* = Expr
  struct Assign : Expr
  {
    List<ID> left;
  };

  // x = var
  struct Var : Assign
  {
    Node<TypeId> type;
    Kind kind() override
    {
      return Kind::Var;
    }
  };

  // x = dup y
  struct Dup : Assign
  {
    Node<ID> y;

    Kind kind() override
    {
      return Kind::Dup;
    }
  };

  // x = load y
  struct Load : Assign
  {
    Node<ID> source;

    Kind kind() override
    {
      return Kind::Load;
    }
  };

  // f ∈ StorageLoc ::= ObjectId × Id
  struct StorageLoc : Value
  {
    // TODO for convenience, can be either oid or name?
    Node<ID> objectid;
    Node<ID> id;

    Kind kind() override
    {
      return Kind::StorageLoc;
    }
  };

  struct Store : Assign
  {
    Node<StorageLoc> y;
    Node<ID> z;

    Kind kind() override
    {
      return Kind::Store;
    }
  };

  enum class LookupType {
    Obj,
    Func,
    Loc,
  }; 

  // lookup type y z
  struct Lookup : Assign
  {
    LookupType type;
    Node<ID> y;
    Node<ID> z;

    Kind kind() override
    {
      return Kind::Lookup;
    }
  };

  // typetest x τ
  struct Typetest : Assign
  {
    Node<ID> y;
    Node<TypeId> type;

    Kind kind() override
    {
      return Kind::Typetest;
    }
  };

  // new Type
  struct NewAlloc : Assign
  {
    Node<TypeId> type;

    Kind kind() override
    {
      return Kind::NewAlloc;
    }
  };

  // stack allocation
  struct StackAlloc : Assign
  {
    Node<TypeId> type;

    Kind kind() override
    {
      return Kind::StackAlloc;
    }
  };

  // x(y*)
  struct Apply
  {
    Node<ID> function;
    List<ID> args;
  };

  // x* = call x(y*)
  struct Call : Assign, Apply
  {
    Kind kind() override
    {
      return Kind::Call;
    }
  };

  // tailcall x(x*)
  struct Tailcall : Expr, Apply
  {
    Kind kind() override
    {
      return Kind::Tailcall;
    }
  };

  // region y(z, z*) TODO(aghosn) not sure I understand this.
  struct Region : Assign, Apply
  {
    Node<Identifier> name;

    Kind kind() override
    {
      return Kind::Region;
    }
  };

  using AllocStrategy = rt::RegionType;
  /* {
     GC,
     Arena,
     RC,
   };*/

  // create Epsi y(z*)
  struct Create : Assign, Apply
  {
    AllocStrategy strategy;

    Kind kind() override
    {
      return Kind::Create;
    }
  };

  struct Branch : Expr
  {
    Node<ID> condition;
    // TODO(aghosn) is that correct?
    // Or should it be apply?
    Node<Apply> branch1;
    Node<Apply> branch2;

    Kind kind() override
    {
      return Kind::Branch;
    }
  };

  // return x*
  struct Return : Expr
  {
    List<ID> returns;

    Kind kind() override
    {
      return Kind::Return;
    }
  };

  // error
  struct Err : Expr
  {
    int a;
    Kind kind() override
    {
      return Kind::Error;
    }
  };

  // x = catch
  struct Catch : Assign
  {
    Kind kind() override
    {
      return Kind::Catch;
    }
  };

  struct Acquire : Expr
  {
    Node<ID> target;

    Kind kind() override
    {
      return Kind::Acquire;
    }
  };

  // release x | release v
  struct Release : Expr
  {
    Node<ID> target;

    Kind kind() override
    {
      return Kind::Release;
    }
  };

  // fulfill x
  struct Fulfill : Expr
  {
    Node<ID> target;

    Kind kind() override
    {
      return Kind::Fulfill;
    }
  };

  struct Freeze : Assign
  {
    Node<ID> target;
    Kind kind() override
    {
      return Kind::Freeze;
    }
  };

  struct Merge : Assign
  {
    Node<ID> target;
    Kind kind() override
    {
      return Kind::Merge;
    }
  };

  /**
   * Types
   */

  struct Iso : TypeRef
  {
    Kind kind() override
    {
      return Kind::Iso;
    }
  };

  struct Mut : TypeRef
  {
    Kind kind() override
    {
      return Kind::Mut;
    }
  };

  struct Imm : TypeRef
  {
    Kind kind() override
    {
      return Kind::Imm;
    }
  };

  struct Paused : TypeRef
  {
    Kind kind() override
    {
      return Kind::Paused;
    }
  };

  struct Stack : TypeRef
  {
    Kind kind() override
    {
      return Kind::Stack;
    }
  };

  struct TypeDecl : Type
  {
    List<TypeId> typeids;
    Map<Id, Member> members;
  };

  struct TypeOp : TypeRef
  {
    Node<TypeRef> left;
    Node<TypeRef> right;
  };

  struct UnionType : TypeOp
  {
    Kind kind() override
    {
      return Kind::UnionType;
    }
  };
  struct IsectType : TypeOp
  {
    Kind kind() override
    {
      return Kind::IsectType;
    }
  };
  struct TupleType : TypeOp
  {
    Kind kind() override
    {
      return Kind::TupleType;
    }
  };

  // store T
  struct StoreType : TypeRef
  {
    Node<TypeRef> type;
    Kind kind() override
    {
      return Kind::StoreMod;
    }

    bool isStorable() override {
      return true;
    }
  };

  struct ClassID : TypeId
  {};

  struct Class : TypeDecl, Expr
  {
    Node<ClassID> id;
    Kind kind() override
    {
      return Kind::Class;
    }
  };
  // Class member field
  struct Field : Member 
  {
    Node<ID> id;
    Node<Type> type;

    Kind kind() override
    {
      return Kind::Field;
    }
  };

  struct ObjectID : Value, Identifier
  {
    Kind kind() override
    {
      return Kind::ObjectID;
    }
  };

  /**
   * TODO(aghosn) introduced this to act as function reference and value;
   */
  struct FunctionID : Value, Identifier
  {
    Kind kind() override
    {
      return Kind::FunctionID;
    }
  };

  struct Function : Apply, Member
  {
    List<Expr> exprs;

    Kind kind() override
    {
      return Kind::Function;
    }
  };

  struct Bool : Constant
  {};
  struct True : Bool
  {
    Kind kind() override
    {
      return Kind::True;
    }
  };
  struct False : Bool
  {
    Kind kind() override
    {
      return Kind::False;
    }
  };
  struct Undef : Constant
  {
    Kind kind() override
    {
      return Kind::Undef;
    }
  };

  // TODO add expressions for program, function, and type fields and methods.
  // Program ::= x(y*): e*
  //        | type x: y* {(Method|Field)*}
  // Method ::= fun x: y
  // Field :: = x

} // namespace verona::ir
