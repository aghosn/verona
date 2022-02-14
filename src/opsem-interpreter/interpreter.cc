#include "interpreter.h"

#include "state.h"
#include "notation.h"
#include "utils.h"
#include "error.h"

#include <cassert>
#include <iostream>
#include <verona.h>

using namespace std;

namespace interpreter {

  static const char* ENTRY_POINT = "main";
  static const int64_t _PC_RESET = -1;
  static const int64_t _PC_START = 0;

Interpreter::Interpreter(ir::Parser* parser) {
  this->parser = parser;
  state.init(parser->classes, parser->functions);
  //TODO set up the entry point?
  if (state.isFunction(ENTRY_POINT)) {
    auto entry = state.getFunction(ENTRY_POINT);
    state.exec_state = {entry->exprs, _PC_START};
  }
  // Setup the fake first frame and the main frame.
  // TODO: figure out whether we need more info in there.
  auto fakeframe = make_shared<Frame>();
  auto mainframe = make_shared<Frame>();
  state.frames.push_back(fakeframe);
  state.frames.push_back(mainframe);

  // Create original region
  auto mainRegion = new (rt::RegionType::Arena) Region;
  state.regions[mainRegion] = rt::RegionType::Arena;
  mainframe->regions.push_back(mainRegion);
  rt::api::open_region(mainRegion);
}

  bool Interpreter::evalOneStep() {
    static int counter = 0;

    CHECK(state.exec_state.offset >= 0, "Invalid PC offset");
    CHECK((state.exec_state.offset == 0 || 
        state.exec_state.offset < state.exec_state.exprs.size()),
        "PC incompatible with the set of expressions available");

    // Stopping condition.
    if (state.exec_state.exprs.size() == 0)
    {
      // Check that the only frame available is the fake one.
      CHECK(state.frames.size() == 1, "Cannot stop with more than 1 frame");
      return true;
    }

    auto pc = state.exec_state.offset;
    auto instr = state.exec_state.exprs[pc];
    cout << "[EVAL]: ";
    this->parser->lexer.dump(instr->tok.la, instr->tok.pos, 0, false);

    try
    {
      instr->accept(this);
    } catch (InterpreterException& e)
    {
      cerr << "[DUMP]: Error line " << instr->tok.la << endl;
      this->parser->lexer.dump(instr->tok.la, instr->tok.pos, 3, true); 
      return true;
    }

    // increase pc 
    state.exec_state.offset++;

    counter++;
    return false;
  }

  void Interpreter::eval() {
    // TODO is that the correct condition?
    bool done = false;
    while((!state.except) && !done)
    {
      done = evalOneStep();
    }
    cout << "[EVAL]: done" << endl;
  }

  void Interpreter::visit(ir::NodeDef* node)
  {
    switch (node->kind())
    {
      case ir::Kind::Var:
        evalVar(node->as<ir::Var>());
        break;
      case ir::Kind::Dup:
        evalDup(node->as<ir::Dup>());
        break;
      case ir::Kind::Load:
        evalLoad(node->as<ir::Load>());
        break;
      case ir::Kind::Store:
        evalStore(node->as<ir::Store>());
        break;
      case ir::Kind::Lookup:
        evalLookup(node->as<ir::Lookup>());
        break;
      case ir::Kind::Typetest:
        evalTypetest(node->as<ir::Typetest>());
        break;
      case ir::Kind::NewAlloc:
        evalNewAlloc(node->as<ir::NewAlloc>());
        break;
      case ir::Kind::StackAlloc:
        evalStackAlloc(node->as<ir::StackAlloc>());
        break;
      case ir::Kind::Call:
        evalCall(node->as<ir::Call>());
        break;
      case ir::Kind::Tailcall:
        evalTailcall(node->as<ir::Tailcall>());
        break;
      case ir::Kind::Region:
        evalRegion(node->as<ir::Region>());
        break;
      case ir::Kind::Create:
        evalCreate(node->as<ir::Create>());
        break;
      case ir::Kind::Branch:
        evalBranch(node->as<ir::Branch>());
        break;
      case ir::Kind::Return:
        evalReturn(node->as<ir::Return>());
        break;
      case ir::Kind::Error:
        evalError(node->as<ir::Err>());
        break;
      case ir::Kind::Catch:
        evalCatch(node->as<ir::Catch>());
        break;
      case ir::Kind::Acquire:
        evalAcquire(node->as<ir::Acquire>());
        break;
      case ir::Kind::Release:
        evalRelease(node->as<ir::Release>());
        break;
      case ir::Kind::Fulfill:
        evalFulfill(node->as<ir::Fulfill>());
        break;
      case ir::Kind::Freeze:
        evalFreeze(node->as<ir::Freeze>());
      case ir::Kind::Merge:
        evalMerge(node->as<ir::Merge>());
      default:
        CHECK(0, "Unknown node being evaluated");
    }
  }

  // x ∉ σ
  // ι ∉ σ
  // --- [var]
  // σ, x = var; e* → σ[ι↦(σ.frame.regions, τᵩ)][x↦(ι, x)], acquire x; e*
  void Interpreter::evalVar(verona::ir::Var& node)
  {
    CHECK(node.left.size() == 1, "Need exactly one x");
    string x = node.left[0]->name;

    // x ∉ σ
    CHECK((!state.isDefinedInFrame(x)), E_NAME_DEF(x));
    CHECK(state.containsType(node.type->name), E_TYPE_NOT_DEF(node.type->name));

    // ι ∉ σ
    //σ[ι↦(σ.frame.regions, τᵩ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    obj->obj = new VObject(); 
    // Setting the object's regions
    for (auto r: state.frames.back()->regions) {
      obj->regions.push_back(r);
    } 
    state.addObject(obj->id, obj);

    // σ[ι↦(σ.frame.regions, τᵩ)][x↦(ι, x)]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 

    // acquire x
    //TODO
  }

  // x ∉ σ
  // ¬iso(σ, σ(y))
  // --- [dup]
  // σ, x = dup y; e* → σ[x↦σ(y)], acquire x; e*
  void Interpreter::evalDup(verona::ir::Dup& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    string y = node.y->name;

    // x ∉ σ
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
   
    // ¬iso(σ, σ(y))
    CHECK(state.isDefinedInFrame(y), E_NAME_NOT_DEF(y));
    auto yvalue = state.frameLookup(y);
    Shared<Object> target = state.getObjectByName(y);
    CHECK(!target->obj->debug_is_iso(), E_CANNOT_BE_ISO);
    
    // σ[x↦σ(y)]
    state.addInFrame(x, state.getValueByName(y));

    // acquire x
    // TODO 
  }

  // x ∉ σ
  // f = σ(y)
  // ¬iso(σ, σ(f))
  // --- [load]
  // σ, x = load y; e* → σ[x↦σ(f)], acquire x; e*
  void Interpreter::evalLoad(verona::ir::Load& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    string y = node.source->name;

    // x ∉ σ
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));

    // f = σ(y)
    // ¬iso(σ, σ(f))
    CHECK(state.isDefinedInFrame(y), E_NAME_NOT_DEF(y));
    auto f = state.frameLookup(y);
    CHECK(f->kind() == ir::Kind::StorageLoc, E_WRONG_KIND(ir::Kind::StorageLoc, f->kind()));

    auto storage = dynamic_pointer_cast<ir::StorageLoc>(f);
    ObjectId oid = storage->objectid->name;
    Id id = storage->id->name;
    CHECK(state.objects.contains(oid), E_OID_NOT_DEF(oid));
    CHECK(state.fields.contains(oid) && state.fields[oid].contains(id),
        E_MISSING_FIELD(oid, id));
    
    auto value = state.fields[oid][id]; 
    // TODO check if this value corresponds to an iso object.

    //  σ[x↦σ(f)]
    state.addInFrame(x, value);

    // acquire x
    // TODO
  }

  // Load a StorageLoc and replace its content in a single step.
  //x ∉ σ
  //norepeat(y; z)
  //f = σ(y)
  //v = σ(z)
  //store(σ, f.id, v)
  //--- [store]
  //σ, x = store y z; e* → σ[f↦v][x↦σ(f)]\{z}, e*
  void Interpreter::evalStore(verona::ir::Store& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    ir::Node<ir::StorageLoc> y = node.y;
    string z = node.z->name;

    //x ∉ σ
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    // TODO norepeat
    //f = σ(y)
    CHECK(state.isDefinedInFrame(y->objectid->name), E_NAME_NOT_DEF(y->objectid->name));
    auto yoid = dynamic_pointer_cast<ir::ObjectID>(state.getValueByName(y->objectid->name)); 
    CHECK(state.fields.contains(yoid->name), E_OID_NOT_DEF(yoid->name));
    CHECK(state.fields[yoid->name].contains(y->id->name), E_MISSING_FIELD(yoid->name, y->id->name));
    auto f = state.fields[yoid->name][y->id->name];

    // Check the field is storable
    // TODO find a better way to implement that.
    auto ytypeName = state.getObjectByName(y->objectid->name)->type; 
    auto ytypeDecl = state.getTypeByName(ytypeName);
    CHECK(ytypeDecl->members.contains(y->id->name), E_MISSING_FIELD(ytypeName, y->id->name));
    auto _field = ytypeDecl->members[y->id->name];
    CHECK(_field->kind() == ir::Kind::Field, E_WRONG_KIND(ir::Kind::Field, _field->kind()));
    auto field = dynamic_pointer_cast<ir::Field>(_field);
    CHECK(field->type->isStorable(), E_NOT_STORABLE((y->id->name)));

    //v = σ(z)
    CHECK(state.isDefinedInFrame(z), E_NAME_NOT_DEF(z));
    auto v = state.frameLookup(z);
    
    // σ[f↦v]
    state.fields[y->objectid->name][y->id->name] = v;

    // [x↦σ(f)]
    state.addInFrame(x, f);
    
    // σ\{z}
    state.removeFromFrame(z);
  }

  // Look in the descriptor table of an object.
  // We can't lookup a StorageLoc unless the object is not iso.
  //x ∉ σ
  //ι = σ(y)
  //m = σ(ι)(z)
  //v = (ι, m) if m ∈ Id
  //    m if m ∈ Function
  //v ∈ StorageLoc ⇒ ¬iso(σ, ι)
  //--- [lookup]
  //σ, x = lookup y z; e* → σ[x↦v], acquire x; e*
  void Interpreter::evalLookup(verona::ir::Lookup& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    string y = node.y->name;
    string z = node.z->name;

    //x ∉ σ
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));

    //ι = σ(y)
    CHECK(state.isDefinedInFrame(y), E_NAME_NOT_DEF(y));
    auto _l = state.getValueByName(y);
    CHECK(_l->kind() == ir::Kind::ObjectID, E_WRONG_KIND(ir::Kind::ObjectID, _l->kind()));
    auto l = dynamic_pointer_cast<ir::ObjectID>(_l);
    
    // m = σ(ι)(z)
    // v = (ι, m)
    CHECK(state.fields.contains(l->name), E_OID_NOT_DEF(l->name));
    CHECK(state.fields[l->name].contains(z), E_MISSING_FIELD(l->name, z));
    auto m = state.fields[l->name][z];

    // v = (ι, m) if m ∈ Id
    //     m if m ∈ Function
    // v ∈ StorageLoc ⇒ ¬iso(σ, ι)
    Shared<ir::Value> v = nullptr;
    switch(m->kind()) {
      case ir::Kind::FunctionID:
        CHECK(node.type == ir::LookupType::Func,
            E_WRONG_LOOKUP(node.type, ir::LookupType::Func));
        v = m;
        break;
      case ir::Kind::StorageLoc:
        CHECK(state.objects[l->name]->obj->debug_is_iso(), E_MUST_BE_ISO);
        CHECK(node.type == ir::LookupType::Loc,
            E_WRONG_LOOKUP(node.type, ir::LookupType::Loc));
        v = m;
        break;
      case ir::Kind::ObjectID:
        CHECK(node.type == ir::LookupType::Obj,
            E_WRONG_LOOKUP(node.type, ir::LookupType::Obj));
        v = m; 
        break;
      default:
        CHECK(0, E_MISSING_CASE);
    }

    // σ[x↦v]
    state.addInFrame(x, v);

    //TODO acquire x
  }


  // Check abstract subtyping.
  // TODO: stuck if not an object?
  //x ∉ σ
  //v = σ(ι) <: τ if ι ∈ ObjectId where ι = σ(y)
  //    false otherwise
  //--- [typetest]
  //σ, x = typetest y τ; e* → σ[x↦v], e*
  void Interpreter::evalTypetest(verona::ir::Typetest& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    string y = node.y->name;
    string tpe = node.type->name;

    // x ∉ σ
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    CHECK(state.containsType(tpe), E_TYPE_NOT_DEF(tpe));
    CHECK(state.isDefinedInFrame(y), E_NAME_NOT_DEF(y));

    // v = σ(ι) <: τ if ι ∈ ObjectId where ι = σ(y)
    Shared<ir::Value> res = make_shared<ir::False>();
    Shared<Object> yobj = nullptr;
    Shared<ir::ObjectID> oid = nullptr;
    auto l = state.frameLookup(y);
    if (l->kind() != ir::Kind::ObjectID) {
      goto end;
    }
    oid = dynamic_pointer_cast<ir::ObjectID>(l);
    if (!state.objects.contains(oid->name))
    {
      goto end;
    }
    yobj = state.objects[oid->name];
    if (yobj->type == tpe) {
      res = make_shared<ir::True>();
    }
    
end:
    state.addInFrame(x, res);
  }

  // Create a new object in the current open region, i.e. a heap object.
  // All fields are initially undefined.
  //x ∉ σ
  //ι ∉ σ
  //σ.frame.regions = (ρ*; ρ)
  //--- [new]
  //σ, x = new τ; e* → σ[ι↦(ρ, τ)][x↦ι], e*
  void Interpreter::evalNewAlloc(verona::ir::NewAlloc& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    string type = node.type->name;

    // x ∉ σ
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    CHECK(state.containsType(type), E_TYPE_NOT_DEF(type));
    
    // ι ∉ σ
    // σ[ι↦(ρ, τ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    //TODO figure out the descriptor.
    obj->obj = new VObject(); 
    // Set up regions
    for (auto r: state.frames.back()->regions) {
      obj->regions.push_back(r);
    }
    state.addObject(obj->id, obj);

    // σ[ι↦(ρ, τ)][x↦ι]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 

    // All fields are initially undefined.
    auto tpe = state.getTypeByName(type);
    for (auto f: tpe->members) {
      //TODO correct this, it can have functions.
      //Filter only the fields.
      state.fields[oid->name][f.first] = make_shared<ir::Undef>();
    }
  }

  // Create a new object in all open regions, i.e. a stack object.
  // All fields are initially undefined.
  // x ∉ σ
  // ι ∉ σ
  // --- [stack]
  // σ, x = stack τ; e* → σ[ι↦(σ.frame.regions, τ)][x↦ι], e*
  void Interpreter::evalStackAlloc(verona::ir::StackAlloc& node)
  {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    string type = node.type->name;
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    CHECK(state.containsType(type), E_TYPE_NOT_DEF(type));

    // ι ∉ σ
    //σ[ι↦(σ.frame.regions, τᵩ)]
    Shared<Object> obj = make_shared<Object>();
    obj->id = nextObjectId(); 
    obj->type = node.type->name;
    obj->obj = new VObject(); 
    // Set up regions
    for (auto r: state.frames.back()->regions) {
      obj->regions.push_back(r);
    }
    state.addObject(obj->id, obj);

    // σ[x↦ι]
    Shared<ir::ObjectID> oid = make_shared<ir::ObjectID>();
    oid->name = obj->id;
    state.addInFrame(x, oid); 

    // All fields are initially undefined.
    auto tpe = state.getTypeByName(type);
    for (auto f: tpe->members) {
      state.fields[oid->name][f.first] = make_shared<ir::Undef>();
    }
  }

  // Push a new frame.
  // norepeat(y; z*)
  // x ∉ σ
  // σ₂, e₂* = newframe(σ₁, (), x*, y, z*, e₁*)
  // --- [call]
  // σ₁, x* = call y(z*); e₁* → σ₂, e₂*
  void Interpreter::evalCall(verona::ir::Call& node)
  {
    // norepeat(y; z*)
    norepeat2(node.args, node.function);
    // x ∉ σ
    for (auto x: node.left) {
      CHECK(!state.isDefinedInFrame(x->name), E_NAME_DEF(x->name));
    }
    auto conts = state.exec_state.getContinuation();
    List<Region*> p;
    auto expr2 = newframe(state, p, node.left, 
        node.function, node.args, conts);
    state.exec_state = {expr2, _PC_RESET};
  }

  // Reuse the current frame.
  // live(σ, x; y*)
  // λ = σ(x)
  // --- [tailcall]
  // σ, tailcall x(y*) → σ[λ.args↦σ(y*)], λ.expr
  void Interpreter::evalTailcall(verona::ir::Tailcall& node) {
    // live(σ, x) && λ = σ(x)
    auto x = node.function->name; 
    auto lambda = state.getFunction(x);

    // live(σ, y*)
    CHECK(node.args.size() == lambda->args.size(),
        E_WRONG_NB(lambda->args.size(), node.args.size()));
    for (auto y: node.args) {
      CHECK(state.isDefinedInFrame(y->name), E_NAME_NOT_DEF(y->name));
    }

    // σ[λ.args↦σ(y*)]
    for (int i = 0; i < node.args.size(); i++) {
      auto name = lambda->args[i]->name;
      auto val = state.frameLookup(node.args[i]->name);
      state.addInFrame(name, val); 
    } 

    // λ.expr
    state.exec_state = {lambda->exprs, _PC_RESET}; 
  }

  // Push a new frame with the specified heap region.
  // norepeat(y; z; z*)
  // x ∉ σ
  // ι = σ(z)
  // ρ = σ(ι).regions
  // iso(σ, ι)
  // σ₂, e₂* = newframe(σ₁, ρ, x*, y, (z; z*), (unpin(σ₁, z*); e₁*))
  // --- [region]
  // σ₁, x* = region y(z, z*); e₁* → σ₂, e₂*
  void Interpreter::evalRegion(verona::ir::Region& node) {
    // norepeat(y; z; z*)
    norepeat2(node.args, node.function);
    // x ∉ σ
    for (auto x: node.left) {
      CHECK(!state.isDefinedInFrame(x->name), E_NAME_DEF(x->name));
    }
    
    string y = node.function->name;
    auto yfunc = state.getFunction(y);

    CHECK(node.args.size() > 0, E_WRONG_NB_AT_LEAST(0, node.args.size()));
    CHECK(node.args.size() == yfunc->args.size(),
        E_WRONG_NB(yfunc->args.size(), node.args.size()));
    for (auto arg: node.args) {
      CHECK(state.isDefinedInFrame(arg->name), E_NAME_NOT_DEF(arg->name));
    }
    
    // ι = σ(z)
    string z = node.args[0]->name;
    auto _l = state.frameLookup(z);
    CHECK(_l->kind() == ir::Kind::ObjectID, E_WRONG_KIND(ir::Kind::ObjectID, _l->kind()));
    auto l = state.getObjectByName(z);

    // ρ = σ(ι).regions
    auto regions = l->regions;

    // iso(σ, ι)
    CHECK(l->obj->debug_is_iso(), E_MUST_BE_ISO);

    // TODO (unpin(σ₁, z*))
    auto conts = state.exec_state.getContinuation();
    auto expr2 = newframe(state, regions, node.left,
        node.function, node.args, conts);
    state.exec_state = {expr2, _PC_RESET};
  } 

  // Create a new heap region.
  // x ∉ σ
  // ρ ∉ σ
  // σ₂, e₂* = newframe(σ₁[ρ↦Σ], ρ, x*, y, z*, (unpin(σ₁, z*); e₁*))
  // --- [create]
  // σ, x* = create Σ y(z*); e* → σ₂, e₂*
  void Interpreter::evalCreate(verona::ir::Create& node) {
    // x ∉ σ
    for (auto x: node.left) {
      CHECK(!state.isDefinedInFrame(x->name), E_NAME_DEF(x->name));
    }

    // get y
    string y = node.function->name;
    auto yfunc = state.getFunction(y);
  
    // ρ ∉ σ
    //TODO I have to find out what to do here, typecast to my own type?
    Region* region = new (node.strategy) Region; 
    
    // σ₁[ρ↦Σ]
    state.regions[region] = node.strategy;

    auto conts = state.exec_state.getContinuation();
    List<Region*> p;
    p.push_back(region);
    //TODO do: unpin(σ₁, z*) 
    auto expr2 = newframe(state, p, node.left, node.function, node.args, conts);
    state.exec_state = {expr2, _PC_RESET};
  }

  // live(σ₁, x; y; z; y*)
  // live(σ₁, x; y; z; z*)
  // λ₁ = σ₁(y)
  // λ₂ = σ₁(z)
  // σ₂, e* = σ₁[λ₁.args↦σ₁(y*)], λ₁.expr if σ₁(x) = true
  //          σ₁[λ₂.args↦σ₁(z*)], λ₂.expr if σ₁(x) = false
  // --- [branch]
  // σ₁, branch x y(y*) z(z*) → σ₂, e*
  void Interpreter::evalBranch(verona::ir::Branch& node) {
    string x = node.condition->name;
    CHECK(state.isDefinedInFrame(x), E_NAME_NOT_DEF(x));
    string y = node.branch1->function->name;
    ir::Node<ir::Function> yfunc = state.getFunction(y);
    CHECK(yfunc->args.size() == node.branch1->args.size(),
        E_WRONG_NB(node.branch1->args.size(), yfunc->args.size()));
    string z = node.branch2->function->name;
    ir::Node<ir::Function> zfunc = state.getFunction(z);
    CHECK(zfunc->args.size() == node.branch2->args.size(),
      E_WRONG_NB(node.branch2->args.size(), zfunc->args.size()));

    // live(σ₁, x; y; z; y*)
    // TODO check that with sylvan
    ir::List<ir::ID> namesY {node.condition/*, node.branch1->function, node.branch2->function*/};
    namesY.insert(namesY.end(), node.branch1->args.begin(), node.branch1->args.end());
    live(state, removeDuplicates(namesY));
    // live(σ₁, x; y; z; z*)
    ir::List<ir::ID> namesZ {node.condition/*, node.branch1->function, node.branch2->function*/};
    namesZ.insert(namesZ.end(), node.branch2->args.begin(), node.branch2->args.end());
    live(state, removeDuplicates(namesZ));
    
    
    //TODO factor check call is well formed.
    ir::Node<ir::Value> xval = state.frameLookup(x);
    bool condition = true;
    switch (xval->kind()) {
      case ir::Kind::True:
        condition = true;
        break;
      case ir::Kind::False:
        condition = false;
        break;
      default:
        CHECK(0, E_MISSING_CASE); 
    }
    if (condition) {
      state.exec_state = {yfunc->exprs, _PC_RESET};
      return;
    } 
    state.exec_state = {zfunc->exprs, _PC_RESET};
  }

  // Pop the current frame.
  // Can only return iso or imm across a `using`.
  // TODO: the isos being returned have to be disjoint
  // live(σ₁, x*)
  // σ₁.frames = (ϕ*; ϕ₁; ϕ₂)
  // σ₂ = ((ϕ*; ϕ₁[ϕ₂.ret↦σ₁(x*)]), σ₁.objects, σ₁.fields, σ₁.except)
  // (ϕ₁.regions ≠ ϕ₂.regions) ⇒ iso(σ₂, ϕ₂(x)) ∨ imm(σ₂, ϕ₂(x))
  // ιs = σ₁.objects(ϕ₂.regions) if ϕ₁.regions ≠ ϕ₂.regions
  //      ∅ otherwise
  // --- [return]
  // σ₁, return x* → σ₂\ιs, ϕ₂.cont
  void Interpreter::evalReturn(verona::ir::Return& node) {
    CHECK(live(state, node.returns), E_LIVELINESS); 
    // σ₁.frames = (ϕ*; ϕ₁; ϕ₂)
    CHECK(state.frames.size() >= 2, 
        E_WRONG_NB_AT_LEAST(1, state.frames.size()));
    // σ₂ = ((ϕ*; ϕ₁[ϕ₂.ret↦σ₁(x*)]), σ₁.objects, σ₁.fields, σ₁.except)
    auto phi1 = state.frames[state.frames.size() - 2];
    auto phi2 = state.frames[state.frames.size() - 1];
    
    bool samereg = sameRegions(phi1->regions, phi2->regions);

    // Discard return values or assign them.
    CHECK(phi2->rets.size() == node.returns.size(), 
        E_WRONG_NB(node.returns.size(), phi2->rets.size()));
    for (int i = 0; i < phi2->rets.size(); i++) {
      auto x = node.returns[i]->name;
      auto ret = phi2->rets[i];
      CHECK(phi2->containsName(x), E_NAME_NOT_DEF(x));
      auto val = phi2->lookup[x];
      CHECK((samereg || isIsoOrImm(state, val)), E_MUST_BE_ISO); 
      phi1->lookup[ret] = phi2->lookup[x];
    }
    
    // Drop the last frame, i.e., phi2
    state.frames.pop_back();

    // ιs = σ₁.objects(ϕ₂.regions) if ϕ₁.regions ≠ ϕ₂.regions
    // ∅ otherwise
    // σ₂\ιs
    if (!samereg) {
      List<ObjectId> ls = getObjectsInRegions(state, phi2->regions);
      for (auto l: ls) {
        state.objects.erase(l);
      }
    }
    state.exec_state = {phi2->continuations, _PC_RESET};
  }

  // Unset the success flag.
  // --- [error]
  // σ, error; e* → (σ.frames, σ.objects, σ.fields, σ.regions, false), e*
  void Interpreter::evalError(verona::ir::Err& node) {
    state.success = false;
  }

  // Test and set the success flag.
  // x ∉ σ₁
  // σ₂ = (σ.frames, σ.objects, σ.fields, σ.regions, true)
  // --- [catch]
  // σ₁, x = catch; e* → σ₂[x↦σ₁.except], e*
  void Interpreter::evalCatch(verona::ir::Catch& node) {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    CHECK(state.except == true, E_UNEXPECTED_VALUE);
    auto val = make_shared<ir::True>();
    state.addInFrame(x, val); 
    //TODO is dat it?
  }

  // TODO: rcinc
  // address: ?
  // object: ?
  // ι = ϕ(x)
  // ω = χ(ι) => ???
  // --- [acquire]
  // σ, acquire x; e* → σ, e*
  void Interpreter::evalAcquire(verona::ir::Acquire& node) {
    string x = node.target->name;
    CHECK(state.isDefinedInFrame(x), E_NAME_NOT_DEF(x));
    auto value = state.frameLookup(x);
    CHECK(value->kind() == ir::Kind::ObjectID,
        E_WRONG_KIND(ir::Kind::ObjectID, value->kind()));
    auto oid = dynamic_pointer_cast<ir::ObjectID>(value);
    CHECK(state.objects.contains(oid->name), E_OID_NOT_DEF(oid->name));
    // TODO ???
  }

  // TODO:
  // if ι is on the stack, destroy it
  // if ι is from an object, rcdec the object
  // ι = ϕ(x)
  // --- [release]
  // σ, release x; e* → σ, e*
  void Interpreter::evalRelease(verona::ir::Release& node) {
    string x = node.target->name;
    CHECK(state.isDefinedInFrame(x), E_NAME_NOT_DEF(x));
    // TODO how do we release it?
    state.removeFromFrame(x);
    // TODO Should remove from objects??
  }

  // TODO: fulfill the promise
  // --- [fulfill]
  // σ, fulfill x → σ, ∅
  void Interpreter::evalFulfill(verona::ir::Fulfill& node) {
    //TODO ???
    //It's an incref basically
  }

  // Destroy a region and freeze all objects that were in it.
  // x ∉ σ
  // ι = σ(y)
  // iso(σ, ι)
  // --- [freeze]
  // σ, x = freeze y; e* → σ[σ(v).regions→∅]\{y}[x↦v], acquire x; e*
  void Interpreter::evalFreeze(verona::ir::Freeze& node) {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    string y = node.target->name;
    CHECK(state.isDefinedInFrame(y), E_NAME_NOT_DEF(y));
    Shared<Object> target = state.getObjectByName(y);
    CHECK(target->obj->debug_is_iso(), E_MUST_BE_ISO);
    
    //rt::Object* res = rt::api::freeze(target);
    //TODO THE HELL IS V? Should be l
  }
  void Interpreter::evalMerge(verona::ir::Merge& node) {
    CHECK(node.left.size() == 1, E_WRONG_NB(1, node.left.size()));
    string x = node.left[0]->name;
    CHECK(!state.isDefinedInFrame(x), E_NAME_DEF(x));
    string y = node.target->name;
    CHECK(state.isDefinedInFrame(y), E_NAME_NOT_DEF(y));
    Shared<Object> target = state.getObjectByName(y);
    CHECK(target->obj->debug_is_iso(), E_MUST_BE_ISO);
    //TODO something with the frame.
    //rt::Object* res = rt::api::merge(target);
    //TODO is that correct?
    //state.addVar(x, res);
  }


} //namespace interpreter
