# Rules

## Questions for Matthew

1. Does a `x = var` acquire or release?
2. `x = stack T` makes the object available in all current regions. What does it mean?
3. x = store y z -> dest value, right? 
4. What is a region?
5. What are the semantics of acquire/release? We lock right?
But it has no notion of waiting or failure.

The var rule -> create an object with field x, and acquire x from x0 before releasing x0.

*extra questions:*

1. norepeat?
2. unpin -> release?

## Notes on notation

what is norepeat
what is unpin

Missing freeze and merge in the grammar.

## Notes one rules

// Allocate a cell on the stack.
// x = var
// ->
// x0 = stack τ
// x = lookup x0 x
// release x0
x ∉ σ
ι ∉ σ
--- [var]
σ, x = var; e* → σ[ι↦(σ.frame.regions, τᵩ)][x↦(ι, x)], acquire x; e*

TODO(@aghosn) is it a release or an acquire?

// Duplicate a stack identifier.
// We can't duplicate an iso value.
x ∉ σ
¬iso(σ, σ(y))
--- [dup]
σ, x = dup y; e* → σ[x↦σ(y)], acquire x; e*

TODO(aghosn): is y an ID or ObjectID 

// Load a StorageLoc and replace its content in a single step.
x ∉ σ
norepeat(y; z)
f = σ(y)
v = σ(z)
store(σ, f.id, v)
--- [store]
σ, x = store y z; e* → σ[f↦v][x↦σ(f)]\{z}, e*

TODO(aghosn)
What is y? i am confused.
Z is the original value, it gets replaced by Y, why introduce f?
f is the value pointed by y in the state? and v ends up in x?
Or the other way around, f now points to v so y is the source.

// Look in the descriptor table of an object.
// We can't lookup a StorageLoc unless the object is not iso.
x ∉ σ
ι = σ(y)
m = σ(ι)(z)
v = (ι, m) if m ∈ Id
    m if m ∈ Function
v ∈ StorageLoc ⇒ ¬iso(σ, ι)
--- [lookup]
σ, x = lookup y z; e* → σ[x↦v], acquire x; e*

@aghosn -> y.z basically, and we need to check if y.z is iso or not.

// Check abstract subtyping.
// TODO: stuck if not an object?
x ∉ σ
v = σ(ι) <: τ if ι ∈ ObjectId where ι = σ(y)
    false otherwise
--- [typetest]
σ, x = typetest y τ; e* → σ[x↦v], e*

@aghosn y must be an ObjectId, we get the typeid of the corresponding object
throught the state and check whether it is indead a submit.


// Create a new object in all open regions, i.e. a stack object.
// All fields are initially undefined.
x ∉ σ
ι ∉ σ
--- [stack]
σ, x = stack τ; e* → σ[ι↦(σ.frame.regions, τ)][x↦ι], e*

@aghosn -> that's what stack means? The object exists in all regions?
What should the region contain?
I though a region was a memory area... how does that work?
Does it mean all regions have a pointer to that object?

// Push a new frame.
norepeat(y; z*)
x ∉ σ
σ₂, e₂* = newframe(σ₁, (), x*, y, z*, e₁*)
--- [call]
σ₁, x* = call y(z*); e₁* → σ₂, e₂*

Pushes a new frame

tailcall is a reuse of the frame.

