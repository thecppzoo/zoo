# The Generic Policy API

## Summary

Description of how to add polymorphic interfaces to `AnyContainer` by assembling *affordances* into the vtable-based generic policy

## Motivation

### `AnyContainer` history

The great software engineer Louis Dionne explains very well what normal C++ polymorphism, based on `virtual` overrides, leaves to be desired in his project [Dyno](https://github.com/ldionne/dyno).

Additionally, in this repository there are a few extra pointers with regards to more subtle needs for polymorphism, in particular sub-typing relationships as in the original meaning of [Barbara Liskov's substitution principle](https://en.wikipedia.org/wiki/Liskov_substitution_principle) that should not force their representation to be sub-classing in C++.  This is better explained in the foundational work by Kevlin Henney, in the [ACCU September 2000 article "From Mechanism to Method: Substitutability"](https://accu.org/index.php/journals/475) and the [C++ Report magazine article with the original implementation of the `any` component](http://www.two-sdg.demon.co.uk/curbralan/papers/ValuedConversions.pdf), which led to `boost::function` and `std::function` and the "mainstreaming" of type erasure as an alternative for polymorphism, this has been a 20 year journey that keeps going. 

One particularly complete way of doing polymorphism (implementing sub-typing relationships) without inheritance and `virtual` overrides of member functions is "type erasure".  Within "type erasure", educator Arthur O'Dwyer has coined the concept of "affordance" to refer to the polymorphic operations a "type erased" object is capable of doing, described in his excellent blog article ["What is Type Erasure?"](https://quuxplusone.github.io/blog/2019/03/18/what-is-type-erasure/).

Without the `AnyContainer` framework, users could still benefit from type erasure (as they do with `std::any` and `std::function`), but then have nothing to help them with specifying their own affordance set, or worse, the stock components may not be compatible at all, for example, if you want the equivalent of a move-only function, neither `std::any` nor `std::function` will help you, at the core of those components there is the affordance of copyability, potentially to a local buffer, and thus copyability of the objects it can hold. This has led to even gigantic organizations in the software engineering industry such as Facebook to write their own specific components such as [`folly::Function`](https://github.com/facebook/folly/blob/master/folly/docs/Function.md).

Type erasure is a very active topic of conceptual development in C++, O'Dwyer reports in ["The space of design choices for `std::function`"](https://quuxplusone.github.io/blog/2019/03/27/design-space-for-std-function/) a large number of proposals around merely `std::function` that are under consideration for addition to the language.

Attempting obtain the benefits of type erasure but with the certainty of maximal performance because of the needs of the industry where I worked at the time, and realizing that work like `folly::Function` are solutions one by one to choices that have obvious generality, I ended up with a complete *conceptualization* of type erasure articulated by applying the Generic Programming paradigm resulting in the framework of `AnyContainer`.

`AnyContainer` already allowed the straightforward codification of **all of the proposals O'Dwyer mentions**, for example, `AnyCallable` indeed is just an straightforward extension of `AnyContainer` and is capable of replacing `std::function` in codebases as large as Snapchat's; the performance, binary size improvements of replacing `std::function` by `AnyCallable` have been hard to believe, yet measured rigorously, attesting to their nearly maximal performance.  Furthermore, move-only containers were implemented, and derived components or extensions such as move only callables were made covariant with respect to copyability maintaining the performance, binary size and clarity improvements, although we now deem that previous work as superceded by the techniques in the latest framework.

However, the framework has been "expert-only". The counterpart of a conceptualization for maximal performance is a conceptualization for ease of use.

### User friendly

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

The container mentioned above is encapsulated as the member `MemoryLayout` of the policy argument to `AnyContainer`.  And the implementation is decided at compilation time (not runtime) through the policy template member `Builder` which takes a type (what the container will hold) and indicates which implementation to activate.  The container or `MemoryLayout` should somehow be capable of "downcasting" to the right implementation at runtime.  This document deals exclusively with a user-made "virtual table pointer" mechanism, I mean not the intrinsic language feature, but a feature implemented in normal C++.

### The concept of affordance

An affordance allows the container to perform operations.  For this, they have to:

1. Introduce some state to the virtual table which is dependent on the type of the object held, this requires:
    1. A default value for this state that will allow accepting any arbitrary type later on
    2. A functional value when the type of the object held is known and the real operation will act on
    3. Both above must be covariant with the actual container (for example, a copy requires knowing how the container is holding the object)
2. Utilize those elements in the virtual table to do the polymorphic operation independently of the other affordances
3. Make available the operation through the container.

#### Generic Programming Concepts about Affordances

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

An affordance may require held-object-type state and also held-object-instance state. For held-object-type state requirements we add something to the virtual table, something of type `VTableEntry`, in the example what we need is not data but the functions to do the copy. Things that are specific to the instance of the object being held are modeled by having access to the object.

The member `Default` is there for default construction, and default construction is present only because it is part of `std::any` and `std::function`.  Default construction just makes an empty instance, that is, one that does not really have something, just prepare it to accept holding a compatible value later, and this is an affordance by itself, with its binary-size penalty, vtable for a defaulted `AnyContainer`.  According to the seminal work "Elements of Programming" by Stepanov and McJones, default construction leads to an object that is only "Partially Formed", something not really useful, that by its presence encourages the antipattern of two-phase initialization; we could save binary space by removing it if we could change the specification of `std::any` and `std::function`, or use different programming techniques, work that could be made later.  When the container realizes its full potential, the access to the affordances is through `Operation`. The *values* of `Default` and `Operation` are relative to the *container implementation* that would really hold the object, they are templates that take the type of container.  The *types* of `Default` and `Operation` **must be assignment-compatible** with the type of the entries in the vtable that the affordance introduces, `VTableEntry`.

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
A `VTableEntry` value templated on a container needed to later accept holding an arbitrary object.  `Default` is allowed to require any API from their containers.

#### `template<typename Container> static VTableEntry Operation`
A `VTableEntry` value templated on a container needed to activate the affordance, also, it is allowed to require anything from their containers.

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

Part of the boilerplate and technicality of user affordances is to make them instance-member-functions of their `AnyContainer`s, but I discourage the use of instance-member-functions in favor of stand-alone functions: Among other things, stand alone functions opt-in into the rich world of function overloads, one of the strongest, most successful parts of C++.

Ideally users would implement their affordances as overloads on `AnyContainer`, either specific or generic policies; what **the *Generic Policy* framework offers** is **the way to inject the essentials of implementation details into the internals of `AnyContainer`**

## Limitations/Future Work

### Covariance of policies and `AnyContainer` and contravariance of the affordance signatures

The Generic Policy framework just described, with some work, could support infinitely recurring substitution:
If a user has a policy `SuperPolicy` and a policy `SubPolicy` in which all the affordances of `SuperPolicy` are also affordances of `SubPolicy`, but `SubPolicy` has some extra affordances, then it should be possible that `AnyContainer<SuperPolicy>` would accept substitutions from `AnyContainer<SubPolicy>`.

According to the Liskov's Substitution Principle, the signatures of the affordances are allowed to be *contravariant* (intuitively "more accepting" or less restricting).

There are two obstacles:
1. Covariance of C++ templates with respect to their template arguments has no explicit support.
2. For many reasons, Object Oriented languages can't support contravariant arguments.

#### Plan to make `AnyContainer` covariant with regards to the subtype relationship of policies

In C++, the only form of type punning allowed is that of the address (or reference) to a derived class to an address (or reference) of a base class.  Thus, to make the `AnyContainer` substitutable:

1. The virtual table between the policies need to have an inheritance relationship
2. `AnyContainer<SubPolicy>` must inherit from `AnyContainer<SuperPolicy>`

I hope to illustrate this with the affordance of copiability: It is straightforward to make a policy with copiability a sub-type of a policy of the same affordances except copiability.  The infrastructure to specify an `AnyContainer<SubPolicy>` to inherit from `AnyContainer<SuperPolicy>` hasn't been done yet, but the internal mechanisms should be compatible.

A much harder problem that **I do not intend to solve at this moment** is that of **multiple subtyping relationships**, that is, multiple inheritance.

#### Contravariance?

I currently lack experience with substitution + contravariance use cases, this will be revisited when needed.

## Annotations

### Why inherit from something that contains the pointer to the virtual table?

According to the rules of the language the state of base classes occur first in the memory layout of the derived objects.  I think for performance reasons the pointer to the vtable should be the first member in the layout.  The only way then is to add it to the container via inheritance, as a member of the first base class.

### Where is the documentation between the specific affordances and the implicit API they require from the containers?

It currently does not exist.  This is the first draft of a mechanism that does not have a precedent in the community, things may change.

### Is it true that a library thing can be better, even have better performance than *native features of the language*?

Alexander Stepanov coined the phrase "relative performance" to mean performance relative to the native features of a language, and "absolute performance" to mean maximal performance.  Most of C++ features lead to object code with *absolute performance*, but runtime polymorphism based on inheritance and `virtual` overrides is manifestly not one.  The language committe insists on not procclaiming that the virtual table pointer mechanism is the implementation for `virtual` instance member function overrides, insists on prohibiting "type-punning", and also that polymorphism is a sub-class relationship; the combination of these rules force that there is very little that can be known about polymorphic objects (for example, the bind between the implementation functions and their objects) and very weak guarantees; this prevents both compilers from optimizations and users to skillfully craft their polymorphic types for maximal performance.

What this framework does is to use mechanisms of maximal performance to synthetize the requirements for runtime polymorphism and uses the Generic Programming Paradigm to implement subtyping relationships without the drawbacks of Object Orientation subclassing of user types.
