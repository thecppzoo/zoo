# Template AnyContainer

## Motivation

In C++ 17 there are new library components `any`, `variant` and `optional` that have existed in the boost libraries for a while.  They have related functionality.  Since `any` can emulate the functionality of `variant` and `optional` this document explains their functionality from the perspective of `any` and will serve as a basis to discuss of `variant` and `optional` in other documents.

`any` allows holding a value of *any* type, hence the name.  It also allows holding no value.  It is conceptually similar to the concept of value or object in JavaScript, (an implementation of something similar is coming here).  `any` allows the applicaton of type-less programming techniques.

There are use cases in which strong typing makes things harder.  For example, a processor of a non-trivial language to specify things like configurations; when the processor attempts to parse a particular configuration in that language, it does not know exactly what is the kind of objects that it will be generating.

This effort is related to a library of callables being developed. Event-based programming interfaces are easier to use if programmers can supply callables of arbitrary types (function pointers, capturing lambdas, etc), the techniques for implementing "type erasure" (concept to be discussed below) here in `any` will be very useful in the event-handling library.

### Alternatives to `any`

In the C family of languages relaxation of type strictness is typically achieved through a `struct` that has an `union` and a field, typically of an integral type, that says what is the type contained in the `union`.  This has a technical name:  "Type erasure" using explicit "type switch" to "discriminate an union".  Another common way is the use of referential semantics (that is, dealing with the values through their addresses as opposed to they themselves) with the intention of using `void *`, the generic pointer type.

In languages such as Java, type relaxation is typically achieved through the "objectification" of values (converting values such as an `int`, which is a primitive type, one of the few types in the Java language that has value semantics, into an `Integer`, an object, that has referential semantics) and using references to a common base class.  In Java all objects inherit from the universal object type, `Object`, and frequently Java programmers use `Object` for this very reason, for example the library of containers:  The container library wants to hold objects of any type, thus containers hold `Object`s.  Also, in Java, as well as other object oriented and strongly typed languages, including C++, the need for type strictness relaxation encourages class hierarchies and the use of references (or pointers) to base classes.  On the other side, languages like Python sometimes are easier to use because of "duck typing".  C++ templates allow all combinations, programmers should be free to use the approach they prefer.  My experience working with JavaScript allowed me to understand that the superimposition of arbitrary taxonomies solely to relax type strictness is pure busy work.

We discuss these options:

1. "Objectification" and making class hierarchies *solely for the purpose of type strictness relaxation*.
2. Using referential semantics plus `void *`.
3. Explicit type switching with discriminated unions
4. Functional/Generic Programming
5. My choice for `any`, which I will name "Value type covariant type hierarchy"

### "Objectification"

Continuing with the example of a configuration language interpreter, the programming language representation for the different types of objects in the specification language may have common properties, *cohesion* that asks for them being represented as variations on a type, hence a taxonomy or class hierarchy may be appropriate.  The requirement of relaxing type stricness should not interfere with the taxonomy or class hierarchy the application domain suggests.

Using inheritance and runtime polymorphism to relax type strictness has several drawbacks.  To begin with, it forces referential semantics (things must be handled through their addresses as opposed to not caring about where they reside in memory), which has the problems of extra indirection, allocation/deallocation of dynamic objects, managing their lifetime, and all the ways in which these things may fail, such as memory exhaustion exceptions, etc.  It also blocks any other alternative to implement type switching (concept to be defined later).

Since support for runtime polymorphism in C++ through inheritance and virtual methods is among the best for comparable languages (for example, it fully supports multiple inheritance of implementations and choices on how to deal with the diamond issue), C++ programmers tend to accept as unavoidable some performance and software development costs associated with using it, however, as shown in the libstdc++, libc++ and my three implementations of `any`, very fine grained control of *type switching* leads to performance and programmer effort economies beyond plain vanilla inheritance-virtual-method.

### `void *`

This choice leads to more programmer effort than any other choice, and leads to error prone code, so it will not be discussed further except to mention it is the easier to understand choice.

### Explicit type switching with discriminated unions

This is a popular choice, it is easy to understand, and requires less effort than `void *`.  It has plenty of applications, including my third implementation of `any`.  This choice may have maximal performance.  There are more abstract and powerful choices:

### Functional/Generic programming

stdlibc++ and libc++ implement type erasure by instantiation of a function that "knows" the nature of the held object.  For the operations they implement a `switch` in the function that does the work of the operation.  I think this choice has close to maximal performance if the space penalty of type erasure is allowed to be that of a function pointer.  The execution time overhead is that of indirect call (calling the function pointer) + indexed jump (jump to the operation) which at least the GCC and Clang optimizers convert into an indirect-indexed call with some operations having dummy arguments.

### Value type covariant type hierarchy

The most negative aspect of shoe-horning a type hierarchy to relax strictness is to interfere with application domain modeling.  Two of the three implementations of `any` made here rely on creating a type hierarchy "parallel" to the values (*covariant*) for the purposes of type switching, hence they don't mess at all with the types of user values.  This choice also has better performance than the Functional/Generic Programming approach because it does not suffer from the argument-type-impedance problem: a single function must have an specific call signature for multiple types of operations, with both programming effort and runtime costs; the creation of polymorphic member functions with their natural signatures is truly maximally performant --note: the "this" implicit argument of virtual calls is not truly an extra argument, explanation below.  Overhead:

1. Indirect (to get the vtable pointer) + indexed call (the index into the vtable for the operation) even without compiler optimizations!, with optimal arguments for all operations: the "this" argument must already reside in a register for a virtual call to take place, this argument has at most the penalty of reducing by one the count of general purpose registers available at the beginning of the function code, truly negligible a penalty.
2. The user choice between
    1. The address of the space for the value handling being implicit in the call, which has superior performance at least in AMD64 (x86-64)
    2. Explicit argument of address of the space, which allows for features such as automatic runtime `any` variant conversions.  These conversions in regular use are very cheap, so, this small runtime cost allows for:
        1. Increased memory efficiency since the programmer can choose a variety of sizes and alignments for `any` that coexist within a program and convert automatically and cheaply.

### Summary

Therefore there is desirability for getting rid of the strictness of types while at the same time not imposing type hierarchies, not forcing users to write type erasure code and preserving advantages of type strictness, especially the object lifetime guarantees, the **RAII** idiom.  Hopefully some advantages of value semantics as well.

The good news is that using `any` may be cheaper, in terms of run-time performance and programming effort, than the alternative of imposing a hierarchy to achieve type strictness relaxation. My implementations, in summary, guarantee that the programmer will be able to fine tune `any` to pay the minimum.

Additionally, the policy-based design of this implementation makes it so that users can write their own policy, such as in the "tight any" implementation demonstrated in this code base.  That is, programmers can use the canonical implementation of `any` while working in parallel to make the canonical policy better for their application by just expressing the improvements as a handful of easy to program primitives, and then use the improved policy without changing the `AnyContainer` using code!

## Type Erasure

The technique for type strictness relaxation is called **type erasure**.  ["Type Erasure"](https://en.wikipedia.org/wiki/Type_erasure) refers to whenever the type of an entity ceases to be compilation-time information and becomes runtime information.  Hence it implies a run-time penalty.  Type erasure can be accomplished in many different ways, all revolving around the concept of **type switching**, any of the mechanisms for discovering the type of an object at run-time.  In C++ type switching is very well supported through the same features that support run-time polymorphism, the "virtual" methods, overrides, which tend to be implemented using the virtual function pointer table technique, the "**vtable**".  To my knowledge, the requirements of C++ virtual methods are universally implemented as that objects begin with the vtable pointer, a hidden member value.  The vtable is the type switch: type-specific features are accessible as indices into the vtable.

Another popular type switching mechanism, applicable to C and C++, is for "structs" to have an explicit type-switch member, which typically will be handled with a `switch` itself: `switch(object.typeId) { case TYPE1: ... }`.  One advantage that sometimes justifies all the extra programming effort for this kind of type switching is that the type switch can be as small as one bit.  Also, some commonality between the types can be achieved in cascading if-then-elses on the type id field.

Curiously, both GCC libstdc++ and Clang's libc++ implement the type erasure needed in `any` by instantiating a template function which "knows" what is the type of the held object and internally has a switch with the many tasks required to manage the held object.  Perhaps the implementers did not use inheritance-virtual-method because of how cumbersome it is to have fine grain control of type switching when using it; however, by not using "polymorphic objects" directly, but working with them as raw bytes I have found fully portable (standard-compliant) ways to have very fine grained control of type switching, leading to my implementation looking relatively straightforward.

Fine-grained type switching control is not possible directly because the inheritance-virtual-method mechanism is in my opinion under-specified in the standard.  For example, determining if two polymorphic objects are of exactly the same type, in any of the standards-compliant ways I know, results in ridiculous code that may explode into calling `strcmp` and all, at least using libstdc++:

```c++
#include <typeinfo>

struct Polymorphic {
    virtual ~Polymorphic() {}
};

bool exactlyTheSameType(
    const Polymorphic &object1,
    const Polymorphic &object2
) {
    return typeid(object1) == typeid(object2);
}
```

[Gets translated to this assembler if using libstdc++](https://godbolt.org/g/9u2egk):

```assembly
exactlyTheSameType(Polymorphic const&, Polymorphic const&): # @exactlyTheSameType(Polymorphic const&, Polymorphic const&)
        mov     rax, qword ptr [rdi]
        mov     rax, qword ptr [rax - 8]
        mov     rcx, qword ptr [rsi]
        mov     rcx, qword ptr [rcx - 8]
        mov     rdi, qword ptr [rax + 8]
        mov     rsi, qword ptr [rcx + 8]
        cmp     rdi, rsi
        je      .LBB0_1
        cmp     byte ptr [rdi], 42
        jne     .LBB0_4
        xor     eax, eax
        ret
.LBB0_1:
        mov     al, 1
        ret
.LBB0_4:
        push    rax
        call    strcmp
        test    eax, eax
        sete    al
        add     rsp, 8
        ret
```

libc++ is better, but still expensive enough that I couldn't recommend putting it in a performance critical loop.  However, determining whether two objects are of the exactly the same type is kind of essential if one wants to have fine-grained control of type switching...

C++ does not prescribe the "vtable" way to support virtual method overrides.  I fail to appreciate the benefit in this, but I see the harm in how hard it is to have fine-grained control of type switching consequence of that.

Careful inspection shows that the more programmer effort-intensive way in which both libstdc++ and libc++ implemented `any` do not perform any better when compared to my two fully compliant implementations, which use C++ runtime polymorphism based on overrides of virtual methods. If anything, I suspect the choice of type switching through a function-with-a-switch leads to code that performs *worse* (more on this later) and also seems error-prone compared to the very straightforward code I wrote.  In any case, my choice is simpler for programmers to use and extend the foundation work I've done.

The key insight to circumvent this problem of lack of fine-grained control of type switching was to use raw bytes.  It is strange that this circumlocutious way to do inheritance leads to performance and also pays for itself in terms of later programming ease.

## Limitations of C++ 17 `any`

`any` is a valuable addition to the standard library.  However, there is a big, obvious performance error in the standard specification that is trivial to fix but neither libc++ nor libstdc++ have fixed, and the implementations of both libc++ and libstdc++ are too rigid to allow any significant improvements.  In particular my implementation allows the programmer to easily make a diversity of performance tradeoff choices, and also more profound variations to make `any` resemble more "value semantics" values or `variant` or `optional`.

### Performance bug in the specification

In the [specification for `any`](http://en.cppreference.com/w/cpp/utility/any), all of the post-construction ways to use the held value through the `any` object, the [overloads of `any_cast`](http://en.cppreference.com/w/cpp/utility/any/any_cast), must perform the check that the type of the held value is the same as the requested type.  This adds to the inherent performance penalty of the type switching necessary.  The fix is as trivial as providing the API to access the value without the check, but neither libc++ nor libstdc++ provide this fix.

Pending concrete benchmarks, I will contact the standards to propose a solution.

### Implementation rigidity

The standard allows and encourages the optimization of holding the value inside the `any` object itself, provided it "fits" inside and its type is no-throw-move-constructible.  Both libc++ and libstdc++ set the largest alignment of the holdable-inside objects to the same of `void *`, and the maximum size [in libc++ to 3 `void *`](http://llvm.org/viewvc/llvm-project/libcxx/trunk/include/any?view=markup&pathrev=300123#l133), [one `void *` in libstdc++](https://github.com/gcc-mirror/gcc/blob/bfe9c13002a83b7a2e992a0f10f279fa6e0d8f71/libstdc%2B%2B-v3/include/std/any#L93).  These parameters in both implementations are rigid, there is absolutely no way to change them except rewriting the code, and even then, the implementation does not support a variety of `any` with different choices.

In my implementation, the alignment and the size are easily user-selectable as `int` template prameters.

In libc++ and libstdc++ `any` is a concrete class, with an implementation of holding the value inside or externally, and no further choices.  In my implementation, `AnyContainer` is a template that receives a policy type containing the programmer choices.  The library supplies a `CanonicalPolicy`  for a default `any` with sensible choices:  The values are held inside the `any` object or indirectly, with the value allocated dynamically; maximum alignment, size for the inside types to be the same as `void *`.  This is expressed in [`RuntimePolymorphicAnyPolicy`](https://github.com/thecppzoo/zoo/blob/45e0075888727ee37900397291b4580c7c3d46ec/inc/util/any.h#L134):

```c++
template<int Size, int Alignment>
struct RuntimePolymorphicAnyPolicy {
    using MemoryLayout = IAnyContainer<Size, Alignment>;

    ...

    template<typename ValueType>
    using Builder =
        typename RuntimePolymorphicAnyPolicyDecider<
            Size,
            Alignment,
            ValueType,
            useValueSemantics<ValueType>()
        >::type;
};
```

The configuration type argument to `AnyContainer` must provide a `MemoryLayout` type and a `template<typename> Builder` that chooses, given an argument type, how to build a value for the given type, this is where, for example, the decision to hold the value internally or referentially.

`MemoryLayout` determines the memory layout characteristics of the instance of `AnyContainer`, as a matter of fact, `AnyContainer` is just a container capable of holding raw bytes with the same size and alignment as `Policy::MemoryLayout`.  All of the implementation of `AnyContainer` is summarized as interpreting the bytes as a `Policy::MemoryLayout` to forward the management of the held object to the implementation handler chosen by `Policy::Builder<ValueType>`

## Comparison with the other two major implementations

At the time of writing only the canonical internal/external implementations have been implemented, but in two varieties:

1. The value container is also polymorphic on the type of value held (see `RuntimePolymorphicAnyPolicy` above)
2. The value container and the type switch are separated.  [The policy](https://github.com/thecppzoo/zoo/blob/45e0075888727ee37900397291b4580c7c3d46ec/inc/util/ExtendedAny.h#L182) (disregard the functions):

```c++
template<int Size, int Alignment>
struct ConverterPolicy {
    using MemoryLayout = ConverterContainer<Size, Alignment>;

    template<typename ValueType>
    using Builder = TypedConverterContainer<Size, Alignment, ValueType>;

    ...
};
```

The choice of making the value container also be a type switch is slightly higher performing than the other choice, because it implicitly "knows" where is the space, when they are separated it must be passed as another parameter to all of the operations.  Since these operations are behind a wall of type-switching indexed call, the compiler has no chance to optimize out the passing of each parameter.  In the AMD64 (x86-64) architecture, for the compiler to be able to put hard-code displacements in the assembler leverages the "SIB" addressing mode, a base register, an scaled index register and an "immediate" displacement, leading for the tighest code (which improves instruction cache effectiveness) and directly raw performance.

Choice #1 is rigid with respect to automatic conversion of `any` of different size, alignments, but choice #2 make it natural.  One well known difficulty of inheritance-runtime-polymorphism is how difficult it is to program templates covariant with class hierarchies, as a matter of fact, one of my greatest annoyances with respect to object orientation in general is that class hierarchies only allow one covariant aspect, the aspect that is super or sub-classified, when there are more aspects covariant, code complexity explodes and performance crashes as with the visitor design pattern.  It is doable, but not practical, to have for a `template<int Size, int Alignment, typename ValueType> struct Thing` to achieve automatic (as in the programmer does not have to supply the template arguments directly) interoperability between `Thing<Size1, Alignment1, Type>` and `Thing<Size2, Alignment2, Type>`.  If the type switch and value-space are separated, then this interoperation becomes practical.

libstc++ and libc++ both follow choice #2, but only [my implementation](https://github.com/thecppzoo/zoo/blob/45e0075888727ee37900397291b4580c7c3d46ec/inc/util/ExtendedAny.h#L201) of #2 allows automatic conversion of `any` of different sizes and alignments.  This automatic conversion relieves the programmer from "Goldilocks" problems of "too big `any` for some types, too small for others" by allowing the size, alignments, that make the most sense in runtime scenarios and easily and cheaply converting from one to the other when necessary.

My implementations are free of conditional branches, what is accomplished with conditional branches in libstdc++ and libc++ is accomplished by properties of types in my implementations.  Not only this is "the proper way to do things, to fully leverage the types to prevent unnecessary runtime costs", but conditional branching is a performance pit that my most recent performance experience proves more important than commonly assumed:  Even when the conditional branching is predictable, it has inherent and increasing costs as processor architectures deepen execution pipelines, plus they tie branch prediction resources that when benchmarking tend to underestimate their costs, because the branch prediction resources are more effective in tiny benchmarks compared to real life programs.

All of these implementations need to make indirect-indexed calls, and for each operation all perform only one, my implementations make the most of these.

### Advantages of `AnyContainer` over `std::any`

1. Users can indenpendently improve the policy to adapt it to their applications without extra work at the places they use AnyContainer.
    1. A policy requires just a handful of easy to program primitives
2. Users are given size and alignment choices
3. Users are given implementations that let them choose very small performance into automatic and cheap convertibility of `AnyContainers` of different sizes and alignments.
4. `AnyContainer` is slightly better performing.

## User-defined `any`

Although the policy mechanism allows for very flexible adaptations, the library already makes the proviso that the `AnyContainer` itself may be modified via inheritance by the user.  The copy and move constructors, as well as the copy and move assignment are inheritance-aware in very subtle SFINAE overloads.  The user still has to take care of the issue of return type covariance, but perhaps I may apply the CRTP (Curiously Recurring Template Pattern) idiom to `AnyContainer` to help with this.

## Details about source code of `AnyContainer`

### Choices about where to put the SFINAE pattern and whether to use auxiliar template parameters

1. **Constructors**: they don't allow the specification of template parameters when called.  Hence, the library is free to use auxiliar template parameters since they are never part of the user interface.  There is no other place to put the SFINAE either.
2. **Member assignment operators**: Even if normally programmers don't call assignment operators with template parameters specified, that well made assignment operator templates must be able to always deduce their template parameters, and that the specification of template parameters to assignments is in my opinion of interest only to experts, I chose to treat assignment operators like normal functions to minimize user interface noise.
3. **Member functions**: The SFINAE pattern is applied in the return value, and there are no auxiliar patterns to prevent noise in the public interfaces.

### Code styles

I have a practical interest in these library components, reflected in some idioms I use, present in this code base, that are not popular.

#### Chaining code

In my code I try to reutilize code as much as possible, not to write less, but for these reasons:

1. Reutilized code is used more than once, it is tested from more than one purpose
2. Code that is hard to reuse typically is bad code, the effort to try to reuse code leads to more clean components
3. Reusing code expresses/forces the behavior to be the same.  For example, the `const` member function should do the same thing as the non-const.  I force this by making the `const` member function a wrapper around the non-const or vice versa.

#### Deconstifying to leverage the non-const versions

Frequently the only difference between the `const` and non-const versions of functions is that they are covariant with respect to the constness of the return type.  I see more danger in copying and pasting the code than in making one call be a wrapper of the other.

#### Inheriting implementations, interfaces with associated data, aggressive use of the CRTP

These things are against common recommendations, in this code they are used because interfaces are not an abstract concept, but an implementation choice with specific memory layouts with carefully controlled subtype variations.

#### Using polymorphic objects through raw bytes

*note: this section had flawed code, which made me get to a potentially wrong conclusion, that placement new may alias, investigating.

#### Further commentary on strict aliasing

*note: this section has been eliminated pending clarification on whether placement new can alias*

## What are the policies? `MemoryLayout` and the value `Builder`s

The implementation of `any` uses a policy type that configures how it does its job.

### `MemoryLayout`

This type determines the memory layout.  An `any` object just contains a `MemoryLayout` object.

This is the interface it must implement, if it were named `Container`:

```c++
    Container() noexcept; // <- no-throw default constructible
    void destroy(); // implicitly noexcept
    void copy(Container *to) const;
    void move(Container *to) noexcept;
    void *value() noexcept;
    bool nonEmpty() const noexcept;
    const std::type_info &type() const noexcept;
    // trivially destructible
```

I believe the interface is self-explanatory.

Please notice that those functions may be virtual, or not, inlined or not, mixed however the programmer sees best.

### `template<typename ValueType> (class|struct|using) Builder`

This is a template that determines the types capable of producing values of the argument type, any `ValueBuilder` must:

1. have this constructor: `template<typename Args...> ValueBuilder(Args &&...)` and use perfect forwarding to the constructor of the value.
2. The memory layout of any value builder must be compatible with that of the `MemoryLayout`.  `any` builds a value only indirectly by building the value builder at the location of `MemoryLayout`.
3. Just as `MemoryLayout`, the value builders must be trivially destructible

### Consequences of inlining

Since this implementation of `any` is header only, and that `any` does not make assumptions about the members of the policy type, users can downgrade the `any` interface by not providing parts of it and not use the features where they are required.  For example, the user may not implement `move` operations if it is not going to use move-constructions, assignments or swapping.

The value builders may restrict implicitly the universe of types suitable for that `any` variant.  For example, if the template `Policy::Builder` only works with `int`, `double`; then those would be the only types of values that the variant `any` could contain.

## Changes under consideration

1. Using the CRTP on the cases where `AnyContainer` methods return an `AnyContainer`, to help inheritance of `AnyContainer`
2. Not requiring `Policy::MemoryLayout` and `Policy::Builder<T>` to be trivially destructible

## Summary

In this implementation of `std::any`:

1. Implements all of the contract for `std::any` that allows to program without imposing arbitrary type hierarchies to relax type strictness
2. Extends the proposal to allow users to make their own tradeoffs, which among other things allows them to minimize almost arbitrarily the performance penalty associated.
    1. Allows a diversity of `any` variations to coexist within the same program
    2. Allows mutually compatible variations
3. The mechanisms for implementing a canonical `any` are re-used for extensions
    1. Users could build their own extensions
        1. Users could inherit from `AnyContainer` and make more profound adaptations
4. Three ways for doing type erasure are presented:
    1. The container of values belongs to a class hierarchy
    2. The value container has a driver in a class hierarchy
    3. Ad-hoc type switching

## Upcoming changes to this document

2. Description of the "tight" policy
