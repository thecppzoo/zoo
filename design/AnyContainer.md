# Template AnyContainer

## Motivation

In C++ 17 there are new library components `any`, `variant` and `optional` that have existed in the boost libraries for a while.  They have related functionality.  Since `any` can emulate the functionality of `variant` and `optional` this document explains their functionality from the perspective of `any` and will serve as a basis to discuss of `variant` and `optional` in other documents.

`any` allows holding a value of *any* type, hence the name.  It also allows holding no value.  It is conceptually similar to the concept of value or object in JavaScript, (an implementation of something similar is coming here).  `any` allows the applicaton of type-less programming techniques.

There are use cases in which strong typing makes things harder.  For example, a processor of a non-trivial language to specify things like configurations; when the processor attempts to parse a particular configuration in that language, it does not know exactly what is the kind of objects that it will be generating.

We are also working in a library of callables, event-based programming interfaces are easier to use if programmers can supply callables of arbitrary types (function pointers, capturing lambdas, etc), the techniques for implementing "type erasure" (concept to be discussed below) here in `any` will be very useful there.

### Alternatives to `any`

In the C family of languages relaxation of type strictness is typically achieved through the use of referential semantics (that is, dealing with the values through their addresses as opposed to they themselves) with the intention of using `void *`, the generic pointer type.

In languages such as Java, type relaxation is typically achieved through the "objectification" of values (converting values such as an `int`, which is a primitive type, one of the few types in the Java language that has value semantics, into an `Integer`, an object, that has referential semantics) and using references to a common base class.  In Java all objects inherit from the universal object type, `Object`, and frequently Java programmers use `Object` for this very reason, for example the library of containers:  The container library wants to hold objects of any type, thus containers hold `Object`s.  Also, in Java, as well as other object oriented and strongly typed languages, including C++, the need for type strictness relaxation encourages class hierarchies and the use of references (or pointers) to base classes.  On the other side, languages like Python sometimes are easier to use because of "duck typing".  C++ templates allow all combinations, programmers should be free to use the approach they prefer.  My experience working with JavaScript allowed me to understand that the superimposition of arbitrary taxonomies solely to relax type strictness is pure busy work.  Using referential semantics plus `void *` leads to just as underperforming code but also ardous and error prone, it is strictly the worst approach available in C++ so I won't discuss it further.

Continuing with the example of a configuration language interpreter, the programming language representation for the different types of objects in the specification language may have common properties, *cohesion* that asks for them being represented as variations on a type, hence a taxonomy or class hierarchy may be appropriate.  The requirement of relaxing type stricness should not interfere with the taxonomy or class hierarchy the application domain suggests.

Using inheritance and runtime polymorphism to relax type strictness has several drawbacks.  To begin with, it forces referential semantics (things must be handled through their addresses as opposed to not caring about where they reside in memory), which has the problems of extra indirection, allocation/deallocation of dynamic objects, managing their lifetime, and all the ways in which these things may fail, such as memory exhaustion exceptions, etc.  It also blocks any other alternative to implement type switching (concept to be defined later).

Since support for runtime polymorphism in C++ through inheritance and virtual methods is among the best for comparable languages (for example, it fully supports multiple inheritance of implementations and choices on how to deal with the diamond issue), C++ programmers tend to accept as unavoidable some performance and software development costs associated with using it, however, as shown in the libstdc++, libc++ and my three implementations of `any`, very fine grained control of *type switching* leads to performance and programmer effort economies beyond plain vanilla inheritance-virtual-method.

Therefore there is desirability for getting rid of the strictness of types while at the same time not imposing type hierarchies and preserving advantges of type strictness, especially the object lifetime guarantees, the **RAII** idiom.  Hopefully some advantages of value semantics as well.

The good news is that using `any` may be performance and programming effort cheaper than the alternative of imposing a hierarchy to achieve type strictness relaxation, my implementations, in summary, guarantee that the programmer will be able to fine tune `any` to pay the minimum.

## Type Erasure

The technique for type strictness relaxation is called **type erasure**.  ["Type Erasure"](https://en.wikipedia.org/wiki/Type_erasure) refers to whenever the type of an entity ceases to be compilation-time information and becomes runtime information.  Hence it implies a run time penalty.  Type erasure can be accomplished in many different ways, all revolving around the concept of **type switching**, any of the mechanisms for discovering the type of an object at runtime.  In C++ type switching is very well supported through the same features that support run time polymorphism, the "virtual" methods, overrides, which tend to be implemented using the virtual function pointer table technique, the "**vtable**".  To my knowledge, the requirements of C++ virtual methods are universally implemented as that objects begin with the vtable pointer, a hidden member value.  The vtable is the type switch: type-specific features are accessible as indices into the vtable.

Another popular type switching mechanism, applicable to C and C++, is for "structs" to have an explicit type-switch member, which typically will be handled with a `switch` itself: `switch(object.typeId) { case TYPE1: ... }`.  One advantage that sometimes justifies all the extra programming effort for this kind of type switching is that the type switch can be as small as one bit.  Also, some commonality between the types can be achieved in cascading if-then-elses on the type id field.

Curiously, both GCC libstdc++ and Clang's libc++ implement the type erasure needed in `any` by instantiating a template function which "knows" what is the type of the held object and internally has a switch with the many tasks required to manage the held object.  Perhaps the implementers did not use inheritance-virtual-method because of how cumbersome it is to have fine grain control of type switching when using it; however, by not using "polymorphic objects" directly, but working with them as raw bytes I have found fully portable (standard-compliant) ways to have very fine grained control of type switching, leading to my implementation looking relatively straightforward.

Why fine-grained type switching control is not possible directly may be because the inheritance-virtual-method mechanism is in my opinion under-specified in the standard, for example, determining if two polymorphic objects are of exactly the same type, using the standard way, results in ridiculous code that may explode into calling `strcmp` and all, at least using libstdc++:

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

libstdc++ is better, but still expensive enough that I couldn't recommend putting it in a performance critical loop.  However, determining whether two objects are of the exactly the same type is kind of essential if one wants to have fine-grained control of type switching...

C++ does not prescribe the "vtable" way to support virtual method overrides.  I fail to appreciate the benefit in this, but I see the harm in how hard it is to have fine-grained control of type switching consequence of that.

Careful inspection shows that the more programmer effort intensive way in which libstdc++ and libc++ implemented `any` compared to my two fully compliant implementations do not perform any better than the choice I used, C++ runtime polymorphism based on overrides of virtual methods, if anything, I suspect the choice of type switching through a function-with-a-switch leads to code that performs *worse* (more on this later) and also seems error prone compared to the very straightforward code I wrote.  In any case, my choice is simpler for programmers to use and extend the foundation work I've done.

The key insight to circumvent this problem of lack of fine-grained control of type switching was to use raw bytes.  It is strange that this circumlocutious way to do inheritance leads to performance and also pays for itself in terms of later programming ease.

## Limitations of C++ 17 `any`

`any` is a valuable addition to the standard library.  However, there is a big, obvious performance error in the standard specification that is trivial to fix but neither libc++ nor libstdc++ have fixed, and the implementations of both libc++ and libstdc++ are too rigid to allow any significant improvements.  In particular my implementation allows the programmer to easily make a diversity of performance tradeoff choices, and also more profound variations to make `any` resemble more "value semantics" values or `variant` or `optional`.

### Performance bug in the specification

In the [specification for `any`](http://en.cppreference.com/w/cpp/utility/any), all of the post-construction ways to use the held value through the `any` object, the [overloads of `any_cast`](http://en.cppreference.com/w/cpp/utility/any/any_cast), must perform the check that the type of the held value is the same as the requested type.  This adds to the inherent performance penalty of the type switching necessary.  The fix is as trivial as providing the API to access the value without the check, but neither libc++ nor libstdc++ provide this fix.

Pending concrete benchmarks, I will contact the standards to propose a solution.

### Implementation rigidity

The standard allows and encourages the optimization of holding the value inside the `any` object itself, provided it "fits" inside and its type is no-throw-move-constructible.  Both libc++ and libstdc++ set the largest alignment of the holdable-inside objects to the same of `void *`, and the maximum size [in libc++ to 3 `void *`](http://llvm.org/viewvc/llvm-project/libcxx/trunk/include/any?view=markup&pathrev=300123#l133), [one `void *` in libstdc++](https://github.com/gcc-mirror/gcc/blob/bfe9c13002a83b7a2e992a0f10f279fa6e0d8f71/libstdc%2B%2B-v3/include/std/any#L93).  These parameters in both implementations are rigid, there is absolutely no way to change them except rewriting the code, and even then, the implementation does not support a variety of `any` with different choices.

In my implementation, the alignment and the size are user-selectable as easily as integer template prameters.

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

All of these implementations need to make indexed calls, and for each operation all perform only one indexed call, my implementation makes the most of these indexed calls.

## User-defined `any`

Although the policy mechanism allows for very flexible adaptations, the library already makes the proviso that the `AnyContainer` itself may be modified via inheritance by the user.  The copy and move constructors, as well as the copy and move assignment are inheritance-aware in very subtle SFINAE overloads.  The user still has to take care of the issue of return type covariance, but perhaps I may apply the CRTP (Curiously Recurring Template Pattern) idiom to `AnyContainer` to help with this.

## Code styles

I have a practical interest in these library components, reflected in some idioms I use, present in this code base, that are not popular.

### Chaining code

In my code I try to reutilize code as much as possible, not to write less, but for these reasons:

1. Reutilized code is used more than once, it is tested from more than one purpose
2. Code that is hard to reuse typically is bad code, the effort to try to reuse code leads to more clean components
3. Reusing code expresses/forces the behavior to be the same.  For example, the `const` member function should do the same thing as the non-const.  I force this by making the `const` member function a wrapper around the non-const or vice versa.

### Deconstifying to leverage the non-const versions

Frequently the only difference between the `const` and non-const versions of functions is that they are covariant with respect to the constness of the return type.  I see more danger in copying and pasting the code than in making one call be a wrapper of the other.

### Inheriting implementations, interfaces with associated data, aggressive use of the CRTP

These things are against common recommendations, in this code they are used because interfaces are not an abstract concept, but an implementation choice with specific memory layouts with carefully controlled subtype variations.

### Using polymorphic objects through raw bytes

Has to do with *strict aliasing*, below.  In general, an `any` in any of its variants needs to be able to change the type of the member that controls the held object, because the `any` needs to reflect the last type assigned or constructed into it.  It is essential, then, to understand how the *strict aliasing rule* allows the compiler to make the assumption (and it actually makes it) that the type of objects in memory never changes (except very few exceptions).  I extended code referenced below to illustrate the issue:

```c++
long strict1(int *ip, long *lp) {
    *lp = 0;
    *ip = 1;
    return *lp;
}

long strict2(char *pi, char *pl) {
    auto lp = reinterpret_cast<long *>(pl);
    *lp = 0;
    *reinterpret_cast<int *>(pi) = 1;
    return *lp;
}

long strict3(void *pi, void *pl) {
    *reinterpret_cast<long *>(pl) = 0;
    *reinterpret_cast<int *>(pi) = 1;
    return *reinterpret_cast<long *>(pl);
}

#include <new>

long notStrict(void *pi, void *pl) {
    new(pl) long(0);
    new(pi) long(1);
    return *reinterpret_cast<long *>(pl);
}

long fool(long &l) {
    return strict3(&l, &l);
}

long smart(long &l) {
    return notStrict(&l, &l);
}
```

The `strict` functions above, with variations on the type of pointers, illustrate how the problem is not only the pointer types but also that the compiler sees that some memory acquires some type and then applies strict aliasing.  The type of the memory behind the pointers is "set" via assignment to `int` and `long`.  Even though an `int` is smaller than a `long`, GCC and Clang both, legitimately, apply the rules of the language in the optimizer to conclude that the pointers can't refer to the same memory (because then that memory would have more than one type) and so decide that the return value of the `long` is not affected by the assignment to the `int`.

As you can see in the [generated assembler](https://godbolt.org/g/WjL6XN),

```assembly
strict1(int*, long*):                         # @strict1(int*, long*)
        mov     qword ptr [rsi], 0
        mov     dword ptr [rdi], 1
        xor     eax, eax
        ret

strict2(char*, char*):                         # @strict2(char*, char*)
        mov     qword ptr [rsi], 0
        mov     dword ptr [rdi], 1
        xor     eax, eax
        ret

strict3(void*, void*):                         # @strict3(void*, void*)
        mov     qword ptr [rsi], 0
        mov     dword ptr [rdi], 1
        xor     eax, eax
        ret

notStrict(void*, void*):                       # @notStrict(void*, void*)
        mov     qword ptr [rsi], 0
        mov     qword ptr [rdi], 1
        mov     rax, qword ptr [rsi]
        ret

fool(long&):                              # @fool(long&)
        mov     qword ptr [rdi], 0
        mov     dword ptr [rdi], 1
        xor     eax, eax
        ret

smart(long&):                             # @smart(long&)
        mov     qword ptr [rdi], 1
        mov     eax, 1
        ret
```

the function `fool` that passes the same pointer as both arguments ends up returning 0 while it should return 1 in little endian where it not for *strict aliasing*, `fool` is free to return 0 or 1 according to the rules, actually, it is *undefined behavior*.  By the way, the compiler does not have to issue the assignments in the order set in the source code, since they refer to different objects, the end result does not depend on which is assigned first! And this may happen if the code is inlined... a good exercise for the reader is to make it so that the compiler sees the advantage of changing the order of assignments given in the source code.

This code base uses one fully portable way to change the type of objects in memory: using `char *` or `char[]` to alias to the  space, according with [basic.lval paragraph 8, numeral 8](http://eel.is/c++draft/basic.lval#8.8) of the standard, which is an exception of strictness, and *in-place-new*.  It is clear that placement new would not make any sense if this operator wasn't an exception to the strict aliasing rules.

**Note**: Please notice how `strict2`, which takes `char *` arguments still activate strict aliasing.  The compilers are right, this is not the same kind of access as described in basic.lval$8.8

**Note**: Readers are encouraged to note the awful code GCC 7.2 and predecessors generate for `notStrict` and `smart`: not only it checks for `nullptr` in placement new, older versions also check for whether a reference is null.

### Further commentary on strict aliasing

There is quite a lot of broken code out there because it breaks the strict aliasing, the authors are not even aware they need building with a compilation option like GCC's, Clang's `-fno-strict-aliasing`.  [This](https://blog.regehr.org/archives/1307)("The Strict Aliasing Situation is Pretty Bad") is a good, concise description of some issues related. Related are problems with type punning, using members not the so-called "active" member in an union.

Personally, I am annoyed by some undefined behavior rules, but not when they lead to more performing code, as strict aliasing.  I recommend, whenever you are sure your code does not rely on third party strict-aliasing broken code, to never disable strict aliasing.

Some people may think strict aliasing is more trouble than it is worth, but that's because of a mindset of using values through their addresses.  A vice.  Take the complications of strict aliasing as a reason to become appreciative of value semantics.

## What are the policies? `MemoryLayout` and the value `Builder`s

The implementation of `any` uses a policy type that configures how it does its job.

### `MemoryLayout`

This type determines the memory layout.  An `any` object just contains a `MemoryLayout` object.

This is the interface it must implement, if it were named `Container`:

```c++
    Container(); // <- default constructible
    void destroy();
    void copy(Container *to);
    void move(ConverterContainer *to) noexcept;
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
