# External Polymorphism and Type Erasure: A Very Useful Dance of Design Patterns

## Motivation for this document

The work of Edward Tufte shows the issues with what we call "slideware".  The subject matter is theoretical enough, that I think we should make a combination of a presentation with a proper long form text for detailed, in depth reading.  In that way, the presentation provides the "big picture" and this resource the details and tighter logical arguments.

### Introduction to substitutability

Consider how effortlessly we use **containers** and **iterators** from the Standard Template Library (STL) to traverse collections of data—whether it’s a `std::vector`, `std::set`, or `std::unordered_set`, despite them being very different containers, we can use the **same iteration logic without concerning ourselves with the details** of the containers and their iterators.  This is "polymorphism", working with things that have very different details through an interface that represents an abstraction that allows us to accomplish useful things.  In the case of iteration, because it is the compiler the thing that resolves the abstract interface to the details of the specific types, we call this "compile-time" polymorphism.

This abstract interface principle that relieves us from having to know the details of the specific objects is also very valuable at *runtime*, especially if our application is going to produce objects that **we don't know what they are** before running the program.  Other programming languages don't have as good compile-time polymorphism as we have in C++, thus, they focus exclusively on runtime polymorphism, to the point that for most of the Software Engineering community "polymorphism" simply means "runtime polymorphism".  Furthermore, for them, polymorphism and **subclassing** are the same thing: the only practical way to get (runtime) polymorphism is via inheritance, and what we call "`virtual` overrides".  This is the fundamental mechanism of abstraction in those languages, Object Oriented Interfaces.  It turns out that in C++ we can still do that, or the option of doing MUCH BETTER.  In the same way that we can achieve ease of programming with containers and iterators that are not related by inheritance, while at the same time getting optimal or nearly optimal performance, for vast practical cases we can devise runtime polymorphism with the same ease of programming and far superior performance and efficiency —much better than through subclassing.  This is what this presentation is about, ultimately, both working less and achieving objectively better performance.

It is not even difficult per se, you'll see in the solutions we have already coded that they are not that complicated, however, what we must do is to ground our work on **fundamentally superior principles** so that our path to do better is revealed.  Let us not be afraid to dive into the theory.

What we aim to achieve is true **substitutability**, as defined by the Liskov Substitution Principle—allowing seamless replacement of types without compromising correctness or efficiency:

### Liskov's substitution, subtypes

**Substitutability**, as defined by the Liskov Substitution Principle (LSP), is the idea that objects of a given more abstract type, let's say, a "container", should be replaceable with objects of a **subtype**, a more specific or concrete type, let's say a `std::vector`, without altering the correctness of the program. In its essence, it ensures that any code working with an abstraction should not need to know the specific details of the underlying concrete implementation—it should "just work" regardless of the actual type used.  Substitutability implies a relation among types: The interface is the supertype and the implementation is the **subtype**.

Of course that subtypes can themselves be supertypes for further refinement.  Notice that this relation is hierarchical, this hierarchical nature is why subtyping is commonly represented in programming through inheritance. However, in real-world modeling, types naturally belong to multiple subtyping relations, which inheritance does not handle well. This rigidity forces unnecessary upfront design decisions that make later adaptations hard.

For example, `std::vector` and `std::map` both map a key to a value (in the case of vectors, the type of key is always an index type), so, they are examples of "mapping containers", however, `std::vector` is an example of a container that stores its data contiguously in memory, while `std::map` store its data as a tree.  So, code that relates to contiguous memory can substitute in a `std::vector`.  We don't think of the many supertypes that something such as `vector` might have because we don't have to make explicit all their subtypes, but when we're using *subclassing*, we must.  That's a huge difference: polymorphism without subclassing admits subtyping relations that are **implicit**.  If we were forced to use subclassing, then we would have to fix types into rigid hierarchies that prevents after-the-fact adaptation. Once a class is written, its relationship to supertypes is set in stone, even if a new, useful subtyping relation is discovered later. This is just one fundamental limitation that motivates inventing other mechanisms.  Supporting polymorphism without subclassing might lead to both higher modeling power and less work.

Now, here’s the exciting part: we can translate powerful compile-time subtyping mechanisms, specific to C++, into the runtime domain.  For example, we will see how relatively easy it is to produce types with both "value semantics" and runtime polymorphism, something that hugely popular languages such as Java and C# fundamentally can't do, no matter the effort.

### Summary

In essence, this presentation is the following:

1. Runtime Polymorphism through subclassing totally sucks.
2. External Polymorphism is how to get (runtime) polymorphism without subclassing: translating compile time polymorphism to runtime.
3. And, if you also want that your "polymorphism without subclassing" would manage the objects, their lifetime, i.e., "*owns*" them, then you probably want "Type Erasure".

### Potential

There's a big problem with External Polymorphism (EP) and Type Erasure (TE): Components like `std::any` and `std::function`, as well as most of the implementations in big foundational libraries such as Facebook's Folly, keep making the same mistakes from 25 years ago, leading to very misleading reputation for both EP and TE, their true potential completely eclipsed.

Bot the errors and the possibilities will be made clear by using superior foundations.  Talking about polymorphism without subclassing is one example.  This separation feels jarring to most colleagues, like clapping with one hand.  And yet, this is what EP does.  That's some epistemic tension that is very useful, I'll show you.  You'll see why I describe "Type Erasure" as "Internal External Polymorphism", in this paradox lies the opportunity to engineer great polymorphism, and then more!

By the end I hope you see how these contrapositions, that empower us, form a dance of design patterns.  Hence the title "EP and TE: A very useful dance of Design Patterns".

### External Polymorphism

#### Innovation chain

External polymorphism is a set of ideas that were floating around by 1997; Chris Cleeland, Douglas C. Schmidt and T. Harrison articulated it and gave it the name.  The most recent version of this paper seems to be Cleeland and Schmidt's "External Polymorphism", readily available at [Schmidt's Vanderbilt](https://www.dre.vanderbilt.edu/~schmidt/PDF/C++-EP.pdf).  This paper does not seem to have been particularly influential, other than having been cited by Kevlin Henney in his very important article "Valued Conversions", that appeared in the [C++ Report magazine in July-August 2000](https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=4610004b383e5c4f2dffbea0019c85847e18fff4)  That is the paper that proposes `any`, which was then followed by Doug McGregor's `boost::function` component, starting in 2001, that is essentially what we have today in the standard library as `std::function`.

#### Giving polymorphism to types that don't have it

I will explain EP in a different way to how it is normally explained:
Adapters for objects so the objects can be used through a runtime-polymorphic interface, without participating in ownership of the objects.

The motivation for EP was to simplify debugging: the authors wanted to easily "dump" or trace the state of objects of arbitrary types, that would not necessarily have that feature of dumping.  Let's rephrase a little bit that requirement in the way most practitioners refer to this, as *serialization*, futhermore, let's be happy to simply "insert" into an `std::ostream`.  Just to make the example more complete, let's say we would also want to be able to get the `std::type_index` of the object.  Notice that we would want to be able to do this even for objects of types such as `int`, `double`, that have no runtime-polymorphism capabilities of their own.

If we knew all the possible types of the objects that we want to give polymorphism to them, we could use `std::variant`, or the older techniques of "type switching" that we still see today in plain C code that deals with polymorphism.

#### EP, first try, as an specific form of "Adapter"

But, if we don't know all the possible subtypes when we are writing the code, we have to do something different that would support an unbounded set of potential subtypes.  Let's start with the [`Adapter`] (https://en.wikipedia.org/wiki/Adapter_pattern) Design Pattern, the normal way EP is presented:

Let's call our interface, the abstraction that will give us the runtime-polymorphism capabilities of insertion to stream and getting the type-index, `SerializableAdapter`, we would get something like this:

WARNING: I am using "modern" language, not the same as in the EP paper.  In modern language, we can also call `SerializableAdapter` as `SerializableConcept`, the "concept" is the abstract interface, that will be "modeled" in an implementation.

```c++
struct SerializableAdapter {
    virtual std::ostream &serialize(std::ostream &) const = 0;
    virtual std::type_index typeIndex() const noexcept = 0;
    virtual ~SerializableAdapter();
    // other members ommitted to focus the explanation
};
```

We create the adapter root, and then proceed to create the implementations for each possible T that we want to be "serializable":

```c++
template<typename T>
struct SerializableModel: SerializableAdapter {
    T *ptr;
    SerializableModel(T *p) noexcept: ptr(p) {}
    std::ostream &serialize(std::ostream &output) const override { return output << *ptr; }
    std::type_index typeIndex() const noexcept override { return typeid(T); }
};
```

Digression: we use "Model" to denote something that "models" the "interface" or "concept".  I think those words have so many meanings, that this context is not good enough to distinguish them from the alternatives.  I think that what is commonly called "model" ought to be called "reification", for making the "abstraction" real or concrete or specific.

We have solved the problem of types such as `int` not being directly "serializable" by creating a mini-hierarchy of one base class, and as many derived classes as different types our codebase makes "serializable".  This is a solution that works, and generally, this is as far as External Polymorphism is explained, in particular, **as far as the EP paper went**.

Notice that the mapping between compile time polymorphism and runtime polymorphism occurs in these two places:
1. `output << *ptr`: this is where overload resolution, and potentially template instantiation of `operator<<`, occurs; this compile-time polymorphism is made available through `virtual` overrides of the "concept" or base class "SerializableAdapter".
2. `typeid(T)` and implicit conversion to `std::type_index`.

#### EP without any subclassing

But if examine this, we are solving the complete lack of runtime polymorphism by synthetizing subclassing.  Fortunately, this is not necessary.

We want something like this, a "concrete" class, without `virtual` members:

```c++
struct Serializable_EP_Concept {
    std::ostream &serialize(std::ostream &) const;
    std::type_index typeIndex() const noexcept;
    // some members ommitted
};
```

We know we can achieve this by imitating the way the language itself implements subclassing:

We can create a virtual table class with all the things needed to implement the interface; and we can make values of type "virtual table", each with the reification for each type:

```c++
struct Serializable_EP_Concept;

struct Serializable_EP_VirtualTable {
    using Concept = Serializable_EP_Concept;
    std::ostream &(*serializationFunctionPointer)(std::ostream &, const Concept *);
    std::type_index (*tiFunctionPointer)() noexcept;
};

struct Serializable_EP_Concept {
    std::ostream &serialize(std::ostream &output) const {
        return this->vTable_->serializationFunctionPointer(output, this);
    }
    std::type_index typeIndex() const noexcept {
        return this->vTable_->tiFunctionPointer();
    }

    const Serializable_EP_VirtualTable *vTable_;
    void *object_;

    template<typename T>
    Serializable_EP_Concept(T *) noexcept;
};

template<typename T>
constexpr Serializable_EP_VirtualTable Serializable_EP_VirtualTable_v = {
    [](std::ostream &output, auto *c) -> std::ostream & {
        return output << static_cast<const T *>(c->object_);
    },
    []() noexcept -> std::type_index { return typeid(T); }
};

template<typename T>
Serializable_EP_Concept::Serializable_EP_Concept(T *p) noexcept:
    vTable_(&Serializable_EP_VirtualTable_v<T>),
    object_(p)
{}
```

You can see that this works in the [Compiler Explorer](https://godbolt.org/z/WKs9GMW3e)

Before analyzing this alternative, we should look closely at what we've done:

We have converted or mapped the compile-time ability of building a value of the virtual table for any stream-insertable type into support for runtime polymorphism of that type!

We can do other things for this mapping.  In particular, we can use a mix of compile time polymorphism and runtime-polymorphism; it depends on exactly what's best in each scenario.  There is a continuum between non-EP Adapters and "pure" EP.  Just like there are many techniques to design&implement Adapters, there are also many for EP.

#### Already getting ahead

Please do not be intimidated by the boilerplate, the need to forward declare and reorder the pieces, because this **polymorphism without subclassing** is already **something that can not be expressed in just about any other programming language** except C++, Rust, D and very few others.

Even more interesting, this beats a conventional polymorphic wrapper in C++, at least in object code size.  If we wrap an `int` like this `SerializableInt`:

```c++
struct ISerializable {
    virtual std::ostream &serialize(std::ostream &) const = 0;
};

struct SerializableInt: ISerializable {
    std::ostream &serialize(std::ostream &o) const { return o << i_; }
    int i_;
};
```

And compare its use to the non-subclassing polymorphism:
```c++
std::ostream &out(std::ostream &o, Serializable_EP_Concept &c) {
    return c.serialize(o);
}

std::ostream &out(std::ostream &o, ISerializable &is) {
    return is.serialize(o);
}
```
We get this assembler (see the example in the compiler explorer link above):
```
out(std::ostream&, Serializable_EP_Concept&):
        mov     rax, QWORD PTR [rsi]
        jmp     [QWORD PTR [rax]]
out(std::ostream&, ISerializable&):
        mov     rdx, QWORD PTR [rsi]
        mov     rax, rdi
        mov     rdi, rsi
        mov     rdx, QWORD PTR [rdx]
        cmp     rdx, OFFSET FLAT:SerializableInt::serialize(std::ostream&) const
        jne     .L8
        mov     esi, DWORD PTR [rsi+8]
        mov     rdi, rax
        jmp     std::ostream::operator<<(int)
.L8:
        mov     rsi, rax
        jmp     rdx
```
Due to GCC's idiosyncracies; if we compile with Clang instead,
```
out(std::ostream&, Serializable_EP_Concept&):
        mov     rax, qword ptr [rsi]
        mov     rax, qword ptr [rax]
        jmp     rax

out(std::ostream&, ISerializable&):
        mov     rax, rdi
        mov     rcx, qword ptr [rsi]
        mov     rcx, qword ptr [rcx]
        mov     rdi, rsi
        mov     rsi, rax
        jmp     rcx
```

Why the difference?

This is important: **because in our External Polymorphism we have the freedom to design the functions in the virtual table for maximum performance, or anything else we care about**, if we use the language feature for polymorphism through subclassing, we have to accept rigid language defaults.

In this example, that the `this` pointer is always the first argument in the ABI, thus, when invoking `serialize`, the first argument is the address of the wrapper.  Immediately invoking `o << i_`, the `ostream` binary interface will require the `ostream` to be the first argument, thus the swap of parameter order must happen.

Knowing this, I deliberately made, at the virtual table definition, that the order would be to first have the `ostream`, and then the External Polymorphism handle.  Despite there being, conceptually, one extra indirection, at the point of invocation the object code is half as long.  The use of EP halves the size of the object code.  Negative cost abstraction.  Of course, once you invoke the function in our vtable then the extra indirection is resolved and the actual invocaton to `operator<<` happens, but this is object code on a per-type basis, if there are 1 million invocations, this EP example would halve the size of each invocation a million times for the cost of the object code of the virtual table for `int` **once**, plus the virtual table for each other type.

### Type Erasure

What if we want our mechanism for EP to also participate in the ownership of the object?

The primordial difference between EP and TE, in my view, is that in TE the wrapper "owns" the object.  For this reason, a lot of interesting things become possible.  The most important, IMO, would be so-called "Value Semantics".  But there may be others.

Just like there is no precise boundary between an application of the Adapter design pattern and External Polymorphism, there is no precise boundary between EP and TE.  One component in this intermediate design space is `std::function_ref`.

For the rest of this document, we will focus on TE when the "wrapper" owns the polymorphic object.

At top level, we can say that, if we add to the EP that implements a polymorphic interface the capabilities to destroy, move, and perhaps copy the object, then we are doing Type Erasure.

And that's enough.

### `any`
There is an interesting twist to this story:  What if we have External Polymorphism of only the ownership capabilities? -- then we would have a container, something like `std::any`, that is not useful by itself, but that has all the runtime polymorphism for ownership.  One way to use that owned object is to be able to see it, this is what `any_cast` does for `std::any`.

Unfortunately, the performance of `any_cast` is terrible, because of design decisions, but we are studying and designing our mechanisms for Type Erasure, then this deficiency in `std::any` and `any_cast` must not prevent us from using the idea when it is useful.

Why would we want to use a container that only give us ownership?

Perhaps because the scenario of subtyping/substitutability is truly complex.

"How would you like to pay for that?" is the way that Kevlin Henney started his article "Valued Conversions", the one in which he invents what eventually became `std::any`.  This ought to be one of the most insightful ledes in all of software design literature.

Keeping only ownership allows us to not have to commit to any superficial subtyping relation.  For example, a "payment" can be money; that would be easy to represent in code.  But it may also be some bartering, that would be harder.  It could still be some other thing, like a payment in labor; and each of these *categories* of payments have myriads of subtypes themselves.  This is when normal subtyping would break down, let alone subclassing.

I think this is paradoxical: removing all runtime polymorphism except what is required for ownership results in greater modeling power!

What if we had one clear subtyping relation with only one function in the interface? that's what the standard library gives us in the form of `std::function`: ownership plus one function in the non-ownership polymorphic interface.

This begs the question, why does it support interfaces of only one function and not two, three, ...?

Because it was not designed properly.

Why is `std::function` not based on `std::any`? because it was not designed properly.

Why do we have no control over the way in which `std::any` and `std::function` own the owned object? (like when the object is local to the container or referenced at the heap, for example)? because of their misdesign.

## Identifying and solving some design issues in Type Erasure implementations

 --in progress--

## Unexplored possibilities

Because in Type Erasure everything that can be done with the object is specified at the level of the polymorphic interface, this gives Type Erasure the freedom to, potentially, not having an object of the same type as what was given at the moment of initialization (construction, assignment), but something else that is capable of having the same behavior, constrained to the polymorphic interface, as the original object type.

I am very interested in "Data Orientation", which requires the conversion of "collections of structs" to "structs of arrays".  I've done work into scattering.

--in progress--
