#pragma once

#include <map>
#include <memory>
#include <vector>

namespace verona::ir {
enum class Kind {
  Invalid,
  Identifier,
  ID,
  TypeID,
  Var,
  Dup,
  Load,
  Store,
  Lookup,
  Typetest,
  NewAlloc,
  StackAlloc,
  Apply,
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
  Interface,
  Class,

  // Constant
  True,
  False,
  Undef,

  End,

};

const char *kindname(Kind k);

struct NodeDef;

// TODO(aghosn) figure out if we need that or not;
template <typename T> using Node = std::shared_ptr<T>;

template <typename T> using List = std::vector<Node<T>>;

template <typename T, typename Y> using Map = std::map<Node<T>, Node<Y>>;

using Ast = Node<NodeDef>;
using AstPath = List<NodeDef>;

/**
 * Everything is a NodeDef.
 *
 * @param location
 *
 * @method kind: returns the kind of this particular node.
 */
struct NodeDef {
  // virtual ~NodeDef() = default;
  virtual Kind kind() = 0;

  template <typename T> T &as() { return static_cast<T &>(*this); }
};

/**
 * Expressions.
 */

struct Expr : NodeDef {};

// TODO(aghosn) avoid double inheritance from the same class.
// so avoid values being express. Let them be just values;
// Try to remove identifier maybe as well.

/**
 * Forward declarations.
 */
// Forward definition
struct Value : Expr {};

/**
 * Identifier
 *
 * @comment: I put it in a struct for the moment in case I need more than just
 * a name. Maybe replace with a constant or make it extend unescaped string?
 */
struct Identifier : Expr {
  std::string name;

  Kind kind() override { return Kind::Identifier; }
};

struct TypeId : Identifier {
  Kind kind() override { return Kind::TypeID; }
};

// Member ::= Function | Id
struct Member : Expr {};

// TODO(aghosn) quick way to change if I fucked up.
// Let's take a litteral approach to the problem.
struct ID : Identifier, Member {

  Kind kind() override { return Kind::ID; }
};

// x* = Expr
struct Assign : Expr {
  List<ID> left;
};

// x = var
struct Var : Assign {
  Kind kind() override { return Kind::Var; }
};

// x = dup y
struct Dup : Assign {
  Node<ID> y;

  Kind kind() override { return Kind::Dup; }
};

// x = load y
struct Load : Assign {
  Node<ID> source;

  Kind kind() override { return Kind::Load; }
};

struct Store : Assign {
  Node<ID> source;
  Node<ID> dest;

  Kind kind() override { return Kind::Store; }
};

// lookup y z
struct Lookup : Assign {
  Node<ID> y;
  Node<ID> z;

  Kind kind() override { return Kind::Lookup; }
};

// typetest x τ
struct Typetest : Assign {
  Node<ID> x;
  Node<TypeId> type;

  Kind kind() override { return Kind::Typetest; }
};

// new Type
struct NewAlloc : Assign {
  Node<TypeId> type;

  Kind kind() override { return Kind::NewAlloc; }
};

// stack Type
struct StackAlloc : Assign {
  Node<TypeId> type;

  Kind kind() override { return Kind::StackAlloc; }
};

// x(y*)
struct Apply : Expr {
  Node<ID> function;
  List<ID> args;

  Kind kind() override { return Kind::Apply; }
};

// x* = call x(y*)
struct Call : Assign, Apply {
  Kind kind() override { return Kind::Call; }
};

// tailcall x(x*)
struct Tailcall : Apply {
  Kind kind() override { return Kind::Tailcall; }
};

/**
 * Scope, several lines of Expression
 */
struct Scope : Expr {
  List<Expr> code;
};

// region y(z, z*) TODO(aghosn) not sure I understand this.
struct Region : Assign, Apply {
  Node<Identifier> name;

  Kind kind() override { return Kind::Region; }
};

enum class AllocStrategy {
  GC,
  RC,
  Arena,
};

// create Epsi y(z*)
struct Create : Assign, Apply {
  AllocStrategy strategy;

  Kind kind() override { return Kind::Create; }
};

struct Branch : Expr {
  Node<ID> condition;
  // TODO(aghosn) is that correct?
  // Or should it be apply?
  Node<Apply> branch1;
  Node<Apply> branch2;

  Kind kind() override { return Kind::Branch; }
};

// return x*
struct Return : Expr {
  List<ID> returns;

  Kind kind() override { return Kind::Return; }
};

// error
struct Err : Expr {
  int a;
  Kind kind() override { return Kind::Error; }
};

// x = catch
struct Catch : Assign {
  Kind kind() override { return Kind::Catch; }
};

struct Acquire : Expr {
  Node<ID> target;

  Kind kind() override { return Kind::Acquire; }
};

// release x | release v
struct Release : Expr {
  Node<ID> target;

  Kind kind() override { return Kind::Release; }
};

// fulfill x
struct Fulfill : Expr {
  Node<ID> target;

  Kind kind() override { return Kind::Fulfill; }
};

/**
 * Constants
 */

struct Constant : Expr {};

/**
 * Types
 */

struct Type : NodeDef {
  List<TypeId> typeids;
  Map<ID, Member> members;
};

struct Iso : Type {
  Kind kind() override { return Kind::Iso; }
};
struct Mut : Type {
  Kind kind() override { return Kind::Mut; }
};
struct Imm : Type {
  Kind kind() override { return Kind::Imm; }
};
struct Paused : Type {
  Kind kind() override { return Kind::Paused; }
};
struct Stack : Type {
  Kind kind() override { return Kind::Stack; }
};

struct TypeOp : Type {
  List<Type> types;
};

struct UnionType : TypeOp {
  Kind kind() override { return Kind::UnionType; }
};
struct IsectType : TypeOp {
  Kind kind() override { return Kind::IsectType; }
};
struct TupleType : TypeOp {
  Kind kind() override { return Kind::TupleType; }
};

//{f: T}-> structural interface, f has member of type T
struct Interface : Type {
  List<Member> members;

  Kind kind() override { return Kind::Interface; }
};

struct Class : Interface {
  Kind kind() override { return Kind::Class; }
};

// store T
struct StoreType : Type {};

// ClassID
struct ClassID : TypeId {};

/**
 * Globals for the execution of the program.
 * TODO(aghosn) Not sure this should be here.
 * I think it does not have to be exprs, it should be stored somewhere else
 */

// TODO(aghosn) I don't get that one.
//ω       ∈ Object      ::= Region* × TypeId
struct Object : Value {
  List<Region> regions;
  Node<TypeId> typeId;
};

struct ObjectId : Value, Identifier {};

// Should be used for values direclty?
// Sounds to me like offset to segment
// i.e., raw data
struct StorageLoc : Value, Identifier {
  Node<ObjectId> objectid;
};

struct Function : Value, Identifier, Member {
  List<ID> ids;
  List<Expr> exprs;

  Kind kind() override { return Kind::Function; }
};

struct Bool : Value, Constant {};
struct True : Bool {
  Kind kind() override { return Kind::True; }
};
struct False : Bool {
  Kind kind() override { return Kind::False; }
};
struct Undef : Constant {
  Kind kind() override { return Kind::Undef; }
};

// TODO(aghosn) instead of havin an id, make it extend a named type.
// TODO(aghosn) maybe have a decl Node too, and make it extended by this.
// Type        ::= TypeId* × (Id → Member)
struct TypeDecl : Type {
  Node<Identifier> typeName;
  Map<Identifier, Member> members;
};

// ϕ       ∈ Frame       ::= Region* × (Id → Value) × Id* × Expression*
struct Frame : NodeDef {
  List<Region> regions;
  Map<Identifier, Value> values;
  // TODO(aghosn) not sure how to parse this.
  // Ids?
  // Expr?
};

struct State : NodeDef {
  List<Frame> frames;
  Map<ObjectId, Object> objects;
  Map<StorageLoc, Value> values;
  Map<Region, AllocStrategy> regions;
  // TODO(aghosn) What the hell is the bool?
};

// TODO(aghosn) do the constants as well.
} // namespace verona::ir
