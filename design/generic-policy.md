# The Generic Policy API

## Summary

Description of how to add polymorphic interfaces to `AnyContainer` by assembling *affordances* into the vtable-based generic policy

## Motivation

The great software engineer Louis Dionne explains very well what normal C++ polymorphism, based on `virtual` overrides, leaves to be desired in his project [Dyno](https://github.com/ldionne/dyno).

Additionally, in this repository there are a few extra pointers with regards to more subtle needs for polymorphism, in particular sub-typing relationships as in the original meaning of [Barbara Liskov's substitution principle](https://en.wikipedia.org/wiki/Liskov_substitution_principle) that should not force their representation to be sub-classing in C++.  This is better explained in the foundational work by Kevlin Henney, in the [ACCU September 2000 article "From Mechanism to Method: Substitutability"](https://accu.org/index.php/journals/475) and the [C++ Report magazine article with the original implementation of the `any` component](http://www.two-sdg.demon.co.uk/curbralan/papers/ValuedConversions.pdf), which led to `boost::function` and `std::function` and the "mainstreaming" of type erasure as an alternative for polymorphism, this has been a 20 year journey that keeps going.

The generally accepted jargon for doing polymorphism (implementing sub-typing relationships) without inheritance and `virtual` overrides of member functions is "type erasure".  Within "type erasure", educator Arthur O'Dwyer has coined the concept of "affordance" to refer to the polymorphic operations a "type erased" object is capable of doing, described in his excellent blog article ["What is Type Erasure?"](https://quuxplusone.github.io/blog/2019/03/18/what-is-type-erasure/).

Type erasure is a very active topic of conceptual development in C++, O'Dwyer reports in ["The space of design choices for `std::function`"](https://quuxplusone.github.io/blog/2019/03/27/design-space-for-std-function/) a large number of proposals around merely `std::function` that are under consideration for addition to the language.

The type erasure framework `AnyContainer` in this repository already allowed the straightforward codification of **all of those proposals**, for example, `AnyCallable` indeed is just an straightforward extension of `AnyContainer` and is capable of replacing `std::function` in codebases as large as Snapchat's; the performance, binary size improvements of replacing `std::function` by `AnyCallable` have been hard to believe, yet measured rigorously.  Furthermore, move-only containers were implemented, and derived components or extensions such as move only callables were made covariant with respect to copyability maintaining the performance, binary size and clarity improvements, but we now deem that previous work as superceded by the techniques in the latest framework.

The latest work on this framework, [`GenericPolicy`](https://github.sc-corp.net/emadrid/szr/blob/6e436e6aaf1d12f2d0992bb3e3f9acf9adec289a/test/inc/zoo/Any/VTablePolicy.h#L204) comes to dramatically simplify the process of developing affordances and defining containers with arbitrary sets of affordances, in which they are specified as trivially as using them as template arguments, the code
```c++
    using Policy = zoo::Policy<zoo::Destroy, zoo::Move, zoo::Copy>;
    using VTA = zoo::AnyContainer<Policy>;
```
indicates a type erasure policy that can do destruction, moving and copying.  In the same way, the user may remove `Copy` to have move-only, or add `RTTI` for normal RTTI, or remove destruction for things that are never destroyed during the lifetime of the process (thus we don't add the binary size of the code to destroy them), implement serialization/deserialization affordances, introspection; the user gains granularity with regards to the mechanisms to hold the type erased objects (for example, on memory arenas based on their types for better memory locality) without having to rewrite massive amounts of error-prone and subtle code.

The objective has been to fully preserve the benefits of type erasure, including performance which is the hardest to keep while doing radical extensions, while simplying use, consistent with the overall doctrine in this repository of encoding profound knowledge into source code that then *gives* performance and clarity.

The affordances of moving and especially copying are probably in the hardest of tiers of affordances to implement because they are fundamental operations of the types, thus, they may have the most complicated *implicit API* with the containers and are a good illustration of the maximum challenge users will be faced when doing their own affordances.

## The implicit contract in an affordance specifier

`AnyContainer` is a wrapper for type erasure mechanisms to acquire the full user interface of `std::any`.  To implement all of the contract of `any`, there is the need of a few key mechanisms:

1. A container type over which the micro-API for type erasure can be activated, this component necessarily must be polymorphic
2. Implementation types that are substitutable to a container, capable of
    1. Actually holding the value
    2. Providing the "knowledge" of the held object to the affordances
3. A type switch mechanism to decide at runtime how to get the right container implementation

The container mentioned above is encapsulated as the member `MemoryLayout` of the policy argument to `AnyContainer`.  And the implementation is decided at compilation time (not runtime) through the policy template member `Builder` which takes a type (what the container will hold) and indicates which implementation to activate.  The container or `MemoryLayout` should somehow be capable of "downcasting" to the right implementation at runtime.  This document deals exclusively with a user-made "virtual table pointer" mechanism.

### The concept of affordance

An affordance allows the container to perform operations.  For this, they have to:

1. Introduce some state to the virtual table which is dependent on the type of the object held, this requires:
    1. A default value for this state that will allow accepting any arbitrary type later on
    2. A functional value when the type of the object held is known and the real operation will act on
    3. Both above must be covariant with the actual container (for example, a copy requires knowing how the container is holding the object)
2. Utilize those elements in the virtual table to do the polymorphic operation independently of the other affordances
3. Make available the operation through the container.

Let us study this affordance example:
```c++
struct Copy {
    struct VTableEntry { void (*cp)(void *, const void *); };

    template<typename Container>
    constexpr static inline VTableEntry Default = { Container::copyVTable };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { Container::copy };

    template<typename Container>
    struct Mixin {
        void copy(void *to) const {
            auto downcast = static_cast<const Container *>(this);
            downcast->template vTable<Copy>()->cp(to, downcast);
        }
    };
};
```

According to the seminal work "Elements of Programming" by Stepanov and McJones, default construction leads to an object that is only "Partially Formed".  In `AnyContainer`, default construction just prepares the instance to accept holding a compatible value, whereas typed-construction realizes the full potential of the container, namely, managing a value on an arbitrary type with the container being able to activate the affordances.  These two levels of initialization are represented or implemented in the generic framework through the members `Default` and `Operation` of each affordance.  The *values* of `Default` and `Operation` are relative to the *container* that would really hold the object, they are templates that take the type of container.  The *types* of `Default` and `Operation` **must be assignment-compatible** with the type of the entries in the vtable that the affordance introduces.

In `Copy`, the affordance adds to the virtual table a function pointer called `cp` that takes a destination pointer and a source pointer, these are the source and destination containers.  `Default` requires the `Container` to provide a class-member `copyVTable`, which should just copy the vtable pointer.  `Operation` requires the `Container` to have a class-member `copy` function.  In this affordance, the actual implementation of how to copy does not reside in the affordance but invokes the container operation; **implicitly it is assumed that the container's `copy` won't be instantiated unless required by this affordance**, in this way, the containers can make a proviso for the possiblity of copying, but not activate it if this affordance is not present, **this is how this affordance controls copyability** *without implementing it*.

In this example, the container implementations and the affordance of copyability are mutually dependent, this is not desirable, but acceptable because there is high cohesion between the ways in which an object can be held and the ways in which it can be copied.  There are some affordances that do not require this.  For example, an affordance of serialization might just require a function pointer in the vtable and a way to get the value held from the container.  A `Type` affordance may just need a `const std::type_info *` in the virtual table and not even access to the object held.

`Copy` has a `Mixin` template member, through CRTP is how the `Container`s acquire the ability of performing the affordance:
```c++
template<typename... Affordances>
struct GenericPolicy {
    struct VTable: Affordances::VTableEntry... {};
    using VTHolder = VTableHolder<VTable>;
    using HoldingModel = void *;

    struct Container: VTHolder, Affordances::template Mixin<Container>... {
```
Because `GenericPolicy<Affordances...>::Container` inherits from `Affordances::template Mixin<Container>`, all public members of `Mixin<Containers>` become public for `Container` too.  In this particular case, the API that is made available is `copy(void *)` whose implementation will activate the `cp` member of the virtual table to perform the container copy, and therefore also the value copy.

### Contract Reference

#### `VTableEntry`
Data type of whatever is needed in the virtual table to implement the affordance

#### `template<typename Container> static VTableEntry Default`
A `VTableEntry` value templated on a container needed to later accept holding an arbitrary object.  `Default` is allowed to require any API from 

#### `template<typename Container> static VTableEntry Operation`
A `VTableEntry` value templated on a container needed to activate the affordance

#### `template<typename Container> Mixin`
The policy's `MemoryLayout` will use the CRTP to inject the public members of `Mixin` into itself, it must contain the declaration and implementation of the affordance, that is, the code that picks from the virtual table the `Operation` and calls it.

## User API affordances

The affordances described thus far are type erasure mechanisms in implementations.

The `AnyContainer` component has a proviso to use affordances provided by the policy in similar way as the `MemoryLayout` of the policies acquire operations from the affordance specifiers: `AnyContainer` CRTPs the type (if it is present) `Affordances` of the policy.

For example, the classical RTTI affordance specifier is currently written as this:

```c++
struct RTTI {
    struct VTableEntry { const std::type_info *ti; };

    template<typename>
    constexpr static inline VTableEntry Default = { &typeid(void) };

    template<typename Container>
    constexpr static inline VTableEntry Operation = {
        &typeid(decltype(*std::declval<Container &>().value()))
    };

    template<typename Container>
    struct Mixin {
        const std::type_info &type() const noexcept {
            auto downcast = static_cast<const Container *>(this);
            return *downcast->template vTable<RTTI>()->ti;
        }
    };

    template<typename AnyC>
    struct UserAffordance {
        const std::type_info &type() const noexcept {
            return static_cast<const AnyC *>(this)->container()->type();
        }
    };
};
```

There is a `UserAffordance` template that takes the user-facing type erasure container, typically `AnyContainer` on a policy that includes this affordance, and does the necessary steps to produce the `type_info` for the held object.

At this stage in development, the way to make use of the user affordances specified in an affordance specifier is through the template `ExtendedAffordancePolicy`:

```c++
template<typename PlainPolicy, typename... AffordanceSpecifications>
struct ExtendedAffordancePolicy: PlainPolicy {
    template<typename AnyC>
    struct Affordances:
        AffordanceSpecifications::template UserAffordance<AnyC>...
    {};
};
```

In the current test file there are several cases of usage of user-facing affordances, including what could be a user-specified affordance called `stringize` which converts the held object to an `std::string`:

From `/test/AnyMovable.cpp`:

```c++
template<typename Container>
std::string stringize(const void *ptr) {
    auto downcast = static_cast<const Container *>(ptr);
    using T = std::decay_t<decltype(*downcast->value())>;
    auto &t = *downcast->value();
    if constexpr(HasToString<T>::value) { return to_string(t); }
    else if constexpr(HasOperatorInsertion<T>::value) {
        std::ostringstream ostr;
        ostr << t;
        return ostr.str();
    }
    else return "";
}

struct Stringize {
    struct VTableEntry { std::string (*str)(const void *); };

    template<typename>
    constexpr static inline VTableEntry Default = {
        [](const void *) { return std::string(); }
    };

    template<typename Container>
    constexpr static inline VTableEntry Operation = { stringize<Container> };

    template<typename Container>
    struct Mixin {};

    template<typename AnyC>
    struct UserAffordance {
        std::string stringize() const {
            auto container =
                const_cast<AnyC *>(static_cast<const AnyC *>(this))->container();
            return container->template vTable<Stringize>()->str(container);
        }
    };
};
```

At the point of use, this suffices:
```c++
zoo::AnyContainer<
    zoo::ExtendedAffordancePolicy<
        zoo::Policy<zoo::Destroy, zoo::Move, Stringize>,
        Stringize
    >
> stringizable;
```

Such code makes a move-only `any` which is capable to convert to string the held objects (by first using the `to_string` function if available for that type or defaulting, if available, to the `operator<<` on an `std::ostream`).

Effectively, this code has implemented an implicit interface "Stringizable" without inheritance nor `virtual` overrides, and the object can be a local variable with the capability of changing type,
```c++
REQUIRE("" == stringizable.stringize());
stringizable = 88;
auto str = stringizable.stringize();
REQUIRE("88" == stringizable.stringize());
Complex c1{0, 1};
auto c2 = c1;
stringizable = std::move(c2);
REQUIRE(to_string(c1) == stringizable.stringize());
```
We can see in that code the object `stringizable` began holding nothing, was locally assigned from an integer, which it was able to convert to string, and then assigned again from a `Complex` type for which `to_string(Complex)` exists and will be used.

At the moment as can be seen there is the need for substantial boilerplate, but I hope through usage we will gain experience to decimate this boilerplate.

## Annotations

### Why inherit from something that contains the pointer to the virtual table?

According to the rules of the language the state of base classes occur first in the memory layout of the derived objects.  I think for performance reasons the pointer to the vtable should be the first member in the layout.  The only way then is to add it to the container via inheritance, as a member of the first base class.

### Where is the documentation between the specific affordances and the implicit API they require from the containers?

It currently does not exist.  This is the first draft of a mechanism that does not have a precedent in the community, things may change.

### Is it true that a library thing can be better, even have better performance than *native features of the language*?

Alexander Stepanov coined the phrase "relative performance" to mean performance relative to the native features of a language, and "absolute performance" to mean maximal performance.  Most of C++ features lead to object code with *absolute performance*, but runtime polymorphism based on inheritance and `virtual` overrides is manifestly not one.  The language committe insists on not procclaiming that the virtual table pointer mechanism is the implementation for `virtual` instance member function overrides, insists on prohibiting "type-punning", and also that polymorphism is a sub-class relationship; the combination of these rules force that there is very little that can be known about polymorphic objects (for example, the bind between the implementation functions and their objects) and very weak guarantees; this prevents both compilers from optimizations and users to skillfully craft their polymorphic types for maximal performance.

What this framework does is to use mechanisms of maximal performance to synthetize the requirements for runtime polymorphism and uses the Generic Programming Paradigm to implement subtyping relationships without the drawbacks of Object Orientation subclassing of user types.
