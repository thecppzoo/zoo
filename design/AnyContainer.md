# Template AnyContainer

## Motivation

In C++ 17 there are new library components `any`, `variant` and `optional` that have existed in the boost libraries for a while.  They have related functionality.  Since `any` can emulate the functionality of `variant` and `optional` this document explains their functionality from the perspective of `any` and will serve the basis for discussions of `variant` and `optional`.

`any` allows holding a value of *any* type, hence the name.  It also allows holding no value.  It is conceptually similar to the concept of value or object in JavaScript.  `any` allows the applicaton of type-less programming techniques.

There are use cases in which strong typing make things harder.  For example, a processor of a non-trivial language to specify configurations; when the processor attempts to parse a particular configuration in that language, it does not know exactly what is the kind of objects that it will be generating.  In the C family of languages relaxation of type strictness is typically achieved through the use of referential semantics (that is, dealing with the values through their addresses as opposed to they themselves) and using `void *`, the generic pointer type.  In languages such as Java, this is typically achieved through the "objectification" of values (converting values such as an `int`, which is a primitive type, into an `Integer`, an object) and using references to a common base class.  In Java all objects inherit from the universal object type, `Object`, and frequently Java programmers use `Object` for this very reason, for example the library of containers.  Also, in Java, as well as other strongly object oriented languages, including C++, this type strictness relaxation encourages class hierarchies and the use of references (or pointers) to base classes.  My experience working with JavaScript allowed me to understand that the superimposition of arbitrary taxonomies to relax type strictness is pure busy work.  On the other hand, using referential semantics plus `void *` leads to just as underperforming code but also ardous and error prone.

Therefore there is desirability for getting rid of the strictness of types while at the same time preserving the advantges of type strictness, especially the object lifetime guarantees, the **RAII** idiom.

`any` is a container of objects of arbitrary type capable of managing their lifetimes and to which RAII fully applies.

## Type Erasure

The technique for type strictness relaxation is called **type erasure**.  ["Type Erasure"](https://en.wikipedia.org/wiki/Type_erasure) refers to whenever the type of an entity ceases to be compilation-time information and becomes runtime information.  Hence it implies a run time penalty.  Type erasure can be accomplished in many different ways, all revolving around the concept of **type switching**, any of the mechanisms for discovering the type of an object at runtime.  In C++ type switching is very well supported through the same features that support run time polymorphism, the "virtual" methods, overrides and the virtual function pointer table, the "**vtable**".  The vtable is, to my knowledge, universally implemented as that objects begin with the vtable pointer, a hidden member value.  The vtable is the type switch: type-specific features are accessible as indices into the vtable.  Another popular type switching mechanism, applicable to C and C++ is to use a type-switch field and have explicit switching, typically with a switch itself: `switch(object.typeId) { case TYPE1: ... }`.  One advantage that sometimes justifies all the extra programming effort for this kind of type switching is that the type switch can be as small as one bit.  Also, some commonality between the types can be achieved in cascading if-then-elses on the type id field.

Curiously, both GCC libstdc++ and Clang's libc++ implement the type erasure needed in `any` by instantiating a template function which "knows" what is the type of the held object and internally has a switch with the many tasks required to manage the held object.  Careful inspection shows this does not perform any better than the choice I used, typical C++ runtime polymorphism based on overrides of virtual methods, if anything, I suspect the choice of type switching through a function-with-a-switch leads to code that performs *worse* and also seems error prone compared to the very straightforward code I wrote.

## Limitations of C++ 17 `any`

`any` is a valuable addition to the standard library.  However, there is a big, obvious performance error in the standard specification that is trivial to fix but neither libc++ nor libstdc++ have fixed, and the implementaitons of both libc++ and libstdc++ are too rigid to allow any significant improvements.  In particular my implementation allows the programmer to easily make a diversity of performance tradeoff choices, and also more profound variations to make `any` resemble more "value semantics" values or `variant` or `optional`.

### Performance bug in the specification

In the [specification for `any`](http://en.cppreference.com/w/cpp/utility/any), all of the post-construction ways to use the held value through the `any` object, the [overloads of `any_cast`](http://en.cppreference.com/w/cpp/utility/any/any_cast), must perform the check that the type of the held value is the same as the requested type.  This adds to the inherent performance penalty of the type switching necessary.  The fix is as trivial as providing the API to access the value without the check, but neither libc++ nor libstdc++ provide this fix.

I will be contacting soon the standards group that owns any to inform them of this.

### Implementation rigidity

The standard allows and encourages the optimization of holding the value inside the `any` object itself, provided it "fits" inside and its type is no-throw-move-constructible.  Both libc++ and libstdc++ set the largest alignment of the holdable-inside objects to the same of `void *`, and the maximum size [in libc++ to 3 `void *`](http://llvm.org/viewvc/llvm-project/libcxx/trunk/include/any?view=markup&pathrev=300123#l133), [one `void *` in libstdc++](https://github.com/gcc-mirror/gcc/blob/bfe9c13002a83b7a2e992a0f10f279fa6e0d8f71/libstdc%2B%2B-v3/include/std/any#L93).  These parameters in both implementations are rigid, there is absolutely no way to change them except rewriting the code, and even then, the implementation does not support a variety of `any` with different choices.

In my implementation, the alignment and the size are user-selectable as easily as integer template prameters.

In libc++ and libstdc++ there can be a single `any`, with an implementation of holding the value inside or externally, and no further choices.  In my implementation, `AnyContainer` is a template that receives a configuration type with the programmer choices.  There is a `CanonicalTypeSwitch`  for a default `any` with sensible choices:  The values are held inside the `any` object or through a pointer, with the value allocated dynamically, maximum alignment, size for the inside types to be the same as `void *`.  This is expressed in [`RuntimePolymorphicImplementation`](https://github.com/thecppzoo/zoo/blob/9a11f1018ebb360d8cb93b763a4ce796dbdf88cd/inc/util/any.h#L117):

```c++
template<int Size_, int Alignment_>
struct RuntimePolymorphicImplementation {
    constexpr static auto Size = Size_;
    constexpr static auto Alignment = Alignment_;

    using Empty = IAnyContainer<Size, Alignment>;

    template<typename ValueType>
    static constexpr bool useValueSemantics() {
        return
            Alignment % alignof(ValueType) == 0 &&
            sizeof(ValueType) <= Size &&
            std::is_nothrow_move_constructible<ValueType>::value;
    }

    template<typename ValueType>
    using Implementation =
        typename RuntimePolymorphicImplementationDecider<
            Size,
            Alignment,
            ValueType,
            useValueSemantics<ValueType>()
        >::type;
};
```

The configuration type argument to `AnyContainer` must provide `Size`, `Alignment`, `Empty` which mean what their names indicate, and a template `Implementation` that chooses, given an argument type, what should be the implementation for it (for example, whether to hold the value internally or referentially).

`Empty` determines the memory layout characteristics of the instance of `AnyContainer`, as a matter of fact, `AnyContainer` is just a container capable of holding raw bytes with the same size and alignment as `TypeSwitch::Empty`.  All of the implementation of `AnyContainer` is summarized as interpreting the bytes as a `TypeSwitch::Empty` to forward the management of the held object to the implementation handler chosen by `TypeSwitch::Implementaiton<ValueType>`

At the time of writing only the canonical internal/external implementations have been implemented.

In Clang's libc++ we can see, at line 141 the different commands:

```c++
141 	  enum class _Action {
142 	    _Destroy,
143 	    _Copy,
144 	    _Move,
145 	    _Get,
146 	    _TypeInfo
147 	  };
```

