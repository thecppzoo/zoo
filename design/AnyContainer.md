# Template AnyContainer

## Motivation

In C++ 17 there are new library components `any`, `variant` and `optional` that have existed in the boost libraries for a while.  They have related functionality.  Since `any` can emulate the functionality of `variant` and `optional` this document explains their functionality from the perspective of `any` and will serve the basis for discussions of `variant` and `optional`.

`any` allows holding a value of *any* type, hence the name.  It also allows holding no value.  It is conceptually similar to the concept of value or object in JavaScript.  `any` allows the applicaton of type-less programming techniques.

There are use cases in which strong typing make things harder.  For example, a processor of a non-trivial language to specify things like configurations; when the processor attempts to parse a particular configuration in that language, it does not know exactly what is the kind of objects that it will be generating.

### Alternatives to `any`

In the C family of languages relaxation of type strictness is typically achieved through the use of referential semantics (that is, dealing with the values through their addresses as opposed to they themselves) with the intention of using `void *`, the generic pointer type.  In languages such as Java, type relaxation is typically achieved through the "objectification" of values (converting values such as an `int`, which is a primitive type, one of the few types in the Java language that has value semantics, into an `Integer`, an object, that has referential semantics) and using references to a common base class.  In Java all objects inherit from the universal object type, `Object`, and frequently Java programmers use `Object` for this very reason, for example the library of containers:  The container library want to hold objects of any type, thus they rely on the type `Object`.  Also, in Java, as well as other strongly object oriented languages, including C++, the need for type strictness relaxation encourages class hierarchies and the use of references (or pointers) to base classes.  My experience working with JavaScript allowed me to understand that the superimposition of arbitrary taxonomies solely to relax type strictness is pure busy work.  On the other hand, using referential semantics plus `void *` leads to just as underperforming code but also ardous and error prone, it is strictly the worst approach available in C++ so I won't discuss it further.

Continuing with the example of a configuration language interpreter, the programming language representation for the different types of objects in the specification language may have common properties, *cohesion* that asks for them being represented as variations on a type, hence a taxonomy or class hierarchy may be appropriate.  The requirement of relaxing type stricness should not interfere with the taxonomy or class hierarchy the application domain suggests.

Using inheritance to relax type strictness and as an option to achieve runtime polymorphism has several drawbacks.  To begin with, it forces referential semantics (things must be handled through their addresses as opposed to not caring about where they reside in memory), which has the problems of extra indirection, allocation/deallocation of dynamic objects, managing their lifetime, and all the ways in which these things may fail.  It also blocks any other alternative to implement type switching (concept to be defined later).

Since support for runtime polymorphism in C++ through inheritance and virtual methods is among the best (for example, it fully supports multiple inheritance of implementations and choices on how to deal with the diamond problem), C++ programmers tend to accept as unavoidable some performance and software development costs associated with using it, however, as shown in the libstdc++, libc++ and my three implementations of `any`, very fine grained control of *type switching* leads to performance and programmer effort economies beyond plain vanilla inheritance-virtual-method.

Therefore there is desirability for getting rid of the strictness of types while at the same time not imposing type hierarchies and preserving advantges of type strictness, especially the object lifetime guarantees, the **RAII** idiom.  Hopefully some advantages of value semantics as well.

Then the good news is that using `any` may be performance and programming effort cheaper than the alternative of imposing a hierarchy to achieve type strictness relaxation, my implementations, in summary, guarantee that the programmer will be able to fine tune `any` to pay the minimum.

## Type Erasure

The technique for type strictness relaxation is called **type erasure**.  ["Type Erasure"](https://en.wikipedia.org/wiki/Type_erasure) refers to whenever the type of an entity ceases to be compilation-time information and becomes runtime information.  Hence it implies a run time penalty.  Type erasure can be accomplished in many different ways, all revolving around the concept of **type switching**, any of the mechanisms for discovering the type of an object at runtime.  In C++ type switching is very well supported through the same features that support run time polymorphism, the "virtual" methods, overrides and the virtual function pointer table, the "**vtable**".  To my knowledge, the requirements of C++ virtual methods are universally implemented as that objects begin with the vtable pointer, a hidden member value.  The vtable is the type switch: type-specific features are accessible as indices into the vtable.  Another popular type switching mechanism, applicable to C and C++ is to use a type-switch field and have explicit switching, typically with a switch itself: `switch(object.typeId) { case TYPE1: ... }`.  One advantage that sometimes justifies all the extra programming effort for this kind of type switching is that the type switch can be as small as one bit.  Also, some commonality between the types can be achieved in cascading if-then-elses on the type id field.

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

Careful inspection shows that the programmer effort more intensive way in which libstdc++ and libc++ implemented `any` compared to my two fully compliant implementations do not perform any better than the choice I used, C++ runtime polymorphism based on overrides of virtual methods, if anything, I suspect the choice of type switching through a function-with-a-switch leads to code that performs *worse* (more on this later) and also seems error prone compared to the very straightforward code I wrote.

The key insight to circumvent this problem of lack of fine-grained control of type switching was to use raw bytes.  It is strange that the circumlocutious way to do inheritance leads to performance and also pays for itself in terms of later programming ease.

## Limitations of C++ 17 `any`

`any` is a valuable addition to the standard library.  However, there is a big, obvious performance error in the standard specification that is trivial to fix but neither libc++ nor libstdc++ have fixed, and the implementaitons of both libc++ and libstdc++ are too rigid to allow any significant improvements.  In particular my implementation allows the programmer to easily make a diversity of performance tradeoff choices, and also more profound variations to make `any` resemble more "value semantics" values or `variant` or `optional`.

### Performance bug in the specification

In the [specification for `any`](http://en.cppreference.com/w/cpp/utility/any), all of the post-construction ways to use the held value through the `any` object, the [overloads of `any_cast`](http://en.cppreference.com/w/cpp/utility/any/any_cast), must perform the check that the type of the held value is the same as the requested type.  This adds to the inherent performance penalty of the type switching necessary.  The fix is as trivial as providing the API to access the value without the check, but neither libc++ nor libstdc++ provide this fix.

Pending concrete benchmarks, I will contact the standards to propose a solution.

### Implementation rigidity

The standard allows and encourages the optimization of holding the value inside the `any` object itself, provided it "fits" inside and its type is no-throw-move-constructible.  Both libc++ and libstdc++ set the largest alignment of the holdable-inside objects to the same of `void *`, and the maximum size [in libc++ to 3 `void *`](http://llvm.org/viewvc/llvm-project/libcxx/trunk/include/any?view=markup&pathrev=300123#l133), [one `void *` in libstdc++](https://github.com/gcc-mirror/gcc/blob/bfe9c13002a83b7a2e992a0f10f279fa6e0d8f71/libstdc%2B%2B-v3/include/std/any#L93).  These parameters in both implementations are rigid, there is absolutely no way to change them except rewriting the code, and even then, the implementation does not support a variety of `any` with different choices.

In my implementation, the alignment and the size are user-selectable as easily as integer template prameters.

In libc++ and libstdc++ `any` is a concrete class, with an implementation of holding the value inside or externally, and no further choices.  In my implementation, `AnyContainer` is a template that receives a policy type containing the programmer choices.  There is a `CanonicalPolicy`  for a default `any` with sensible choices:  The values are held inside the `any` object or through a pointer, with the value allocated dynamically, maximum alignment, size for the inside types to be the same as `void *`.  This is expressed in [`RuntimePolymorphicAnyPolicy`](https://github.com/thecppzoo/zoo/blob/50f500fc02cda844234b7d0cbcf887946380883c/inc/util/any.h#L134):

```c++
template<int Size, int Alignment>
struct RuntimePolymorphicAnyPolicy {
    using Empty = IAnyContainer<Size, Alignment>;

    template<typename ValueType>
    static constexpr bool useValueSemantics() {
        return canUseValueSemantics<ValueType>(Size, Alignment);
    }

    template<typename ValueType>
    using Implementation =
        typename RuntimePolymorphicAnyPolicyDecider<
            Size,
            Alignment,
            ValueType,
            useValueSemantics<ValueType>()
        >::type;
};
```

The configuration type argument to `AnyContainer` must provide an `Empty` type and a template `Implementation` that chooses, given an argument type, what should be the implementation for it (for example, whether to hold the value internally or referentially).

`Empty` determines the memory layout characteristics of the instance of `AnyContainer`, as a matter of fact, `AnyContainer` is just a container capable of holding raw bytes with the same size and alignment as `Policy::Empty`.  All of the implementation of `AnyContainer` is summarized as interpreting the bytes as a `Policy::Empty` to forward the management of the held object to the implementation handler chosen by `Policy::Implementaiton<ValueType>`

## Comparison with the other two major implementations

At the time of writing only the canonical internal/external implementations have been implemented, but in two varieties:

1. The value container is also polymorphic on the type of value held (see `RuntimePolymorphicAnyPolicy` above)
2. The value container and the type switch are separated.  [The policy](https://github.com/thecppzoo/zoo/blob/2560c3a3c314ecff34188ac58d21847ca9aacb22/inc/util/ExtendedAny.h#L182) (disregard the functions):

```c++
template<int Size, int Alignment>
struct ConverterPolicy {
    using Empty = ConverterContainer<Size, Alignment>;

    template<typename ValueType>
    using Implementation = TypedConverterContainer<Size, Alignment, ValueType>;

    template<typename V>
    bool isRuntimeValue(Empty &e) {
        return dynamic_cast<ConverterValue<V> *>(e.driver());
    }

    template<typename V>
    bool isRuntimeReference(Empty &e) {
        return dynamic_cast<ConverterReferential<V> *>(e.driver());
    }
};
```

The choice of making the value container also be a type switch is slightly higher performing than the other choice, because it implicitly "knows" where is the space, when they are separated it must be passed as another parameter to all of the operations.  Since these operations are behind a wall of type-switching, the compiler has no chance to optimize out the passing of each parameter.

Choice #1 is rigid with respect to automatic conversion of `any` of different size, alignments, but choice #2 make it natural.

libstc++ and libc++ both follow choice #2, but only my implementation of #2 allows automatic conversion of `any` of different sizes and alignments.

My implementations are free of conditional branches, what is accomplished with conditional branches in libstdc++ and libc++ is accomplished by properties of types in my implementations.  All of these implementations need to make indexed calls, and for each operation all perform only one indexed call.  Meaning my implementations are better performing.

## Summary

In this implementation of `std::any`:

1. Implements all of the contract for `std::any` that allows to program without imposing arbitrary type hierarchies to relax type strictness
2. Extends the proposal to allow users to make their own tradeoffs, which among other things allows them to minimize almost arbitrarily the performance penalty associated.
    1. Allows a diversity of `any` variations to coexist within the same program
    2. Allows mutually compatible variations
3. The mechanisms for implementing a canonical `any` are re-used for extensions
    1. Users could build their own extensions
4. Three ways for doing type erasure are presented:
    1. The container of values belongs to a class hierarchy
    2. The value container has a driver in a class hierarchy
    3. Ad-hoc type switching
