#pragma once

#include <map>
#include <vector> 

#include "source.h"

namespace verona::ir {
  enum class Kind {
  };

  struct NodeDef;

  template<typename T>
  using Node = std::shared_ptr<T>;

  template<typename T>
  using List = std::vector<Node<T>>;

  template<typename T, typename Y>
  using Map = std::map<Node<T>, Node<Y>>;



/**
 * Everything is a NodeDef.
 *
 * @param location
 *
 * @method kind: returns the kind of this particular node.
 */
  struct NodeDef {
    verona::parser::Location location;

    virtual ~NodeDef() = default;
    virtual Kind kind() = 0;

    template<typename T>
    T& as() 
    {
      return static_cast<T&>(*this);
    }
  }; 

 /**
  * Expressions.
  */

  struct Expr: NodeDef
  {};

  /**
   * Forward declarations.
   */
    // Forward definition
  struct Value : Expr {} ;
  struct Type;

  /**
   * Identifier
   *
   * @comment: I put it in a struct for the moment in case I need more than just
   * a name.
   */
  struct Identifier : Expr {
    std::string name;
  };
  
  // x = y
  struct Assign: Expr {
    Node<Expr> left;
    Node<Expr> right;
  };

 
  // store y z
  struct Load: Expr {
    Node<Expr> y;
  };

  struct Store: Expr {
    Node<Expr> source;
    Node<Expr> dest;
  };

  // lookup y z
  struct Lookup: Expr {
    Node<Expr> y;
    Node<Expr> z;
  };

  // typetest x τ
  struct Typetest: Expr {
    Node<Expr> x;
    Node<Type> type;
  };

  // new Type
  struct NewAlloc: Expr {
    Node<Type> type;
  };

  // stack Type
  struct StackAlloc: Expr {
    Node<Type> type;
  };

  // call x(y*)
  struct Call: Expr {
    Node<Identifier> function;
    List<Expr> args;
  };

  // tailcall x(x*)
  struct Tailcall : Call {
  };

  /**
   * Scope, several lines of Expression
   */
  struct Scope: Expr {
    List<Expr> code;
  };


  // region y(z, z*) TODO(aghosn) not sure I understand this.
  struct Region: Scope {
    Node<Identifier> name;
  };

  enum class AllocStrategy {
    GC,
    RC,
    Arena,
  };

  // create Epsi y(z*)
  struct Create: Scope {
    AllocStrategy strategy; 
  };

  
  struct Branch: Expr {
    Node<Expr> condition;
    // TODO(aghosn) is that correct?
    Node<Expr> branch1;
    Node<Expr> branch2;
  };

  struct Return: Expr {
    Node<Expr> value;
  };

  struct Acquire: Expr {
    Node<Expr> target;
  };

  struct Release: Expr  {
    Node<Expr> target;
  };

  struct Fulfill: Expr {
    Node<Expr> target;
  };

  struct Member : Expr {
    //TODO(aghosn) How to get reference to parent?
  };
  

  struct Function : Member, Value {
    Node<Identifier> name;
    List<Type> typeparams;
    // TODO(aghosn) should it be identifiers?
    List<Expr> args;
    Node<Type> result; 
    Node<Expr> body; 
  };

  /**
   * Constants
   */
  
  struct Constant: Expr {};

  /**
   * Type
   */
  struct Type: NodeDef
  {};

  struct Iso : Type {};
  struct Mut : Type {};
  struct Imm: Type {};
  struct Paused: Type {};
  struct Stack: Type {};

  /**
   * Globals for the execution of the program.
   * TODO(aghosn) Not sure this should be here. 
   * I think it does not have to be exprs, it should be stored somewhere else
   */

  // TODO(aghosn) I don't get that one.
  //ω       ∈ Object      ::= Region* × TypeId
  struct Object : Value {
    List<Region> regions;
    Node<Type> typeId;
  };

  struct ObjectId : Value, Identifier {
    
  };

  // Should be used for values direclty?
  // Sounds to me like offset to segment
  // i.e., raw data
  struct StorageLoc : Value, Identifier {
    Node<ObjectId> objectid;
  };

  //TODO(aghosn) instead of havin an id, make it extend a named type.
  //TODO(aghosn) maybe have a decl Node too, and make it extended by this.
  //Type        ::= TypeId* × (Id → Member)
  struct TypeDecl : Type {
    Node<Identifier> typeName;
    Map<Identifier, Member> members;
  };

  // ϕ       ∈ Frame       ::= Region* × (Id → Value) × Id* × Expression* 
  struct Frame :  NodeDef {
    List<Region> regions;
    Map<Identifier, Value> values;
    //TODO(aghosn) not sure how to parse this.
    //Ids?
    //Expr?
  }; 

  struct State : NodeDef {
    List<Frame> frames;
    Map<ObjectId, Object> objects;
    Map<StorageLoc, Value> values;
    Map<Region, AllocStrategy> regions;
    // TODO(aghosn) What the hell is the bool?
  }; 


  //TODO(aghosn) do the constants as well. 
}

