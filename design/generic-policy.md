# The Generic Policy API

## Summary

Description of how to add polymorphic interfaces to `AnyContainer` by assembling *affordances* into the vtable-based generic policy

## Motivation

The great software engineer Louis Dionne explains very well what normal C++ polymorphism, based on `virtual` overrides, leaves to be desired in his project [Dyno](https://github.com/ldionne/dyno).

Additionally, in this repository there are a few extra pointers with regards to more subtle needs for polymorphism, in particular sub-typing relationships as in the original meaning of [Barbara Liskov's substitution principle](https://en.wikipedia.org/wiki/Liskov_substitution_principle) that should not force their representation to be sub-classing in C++.  This is better explained in the foundational work by Kevlin Henney, in the [ACCU September 2000 article "From Mechanism to Method: Substitutability"](https://accu.org/index.php/journals/475) and the [C++ Report magazine article with the original implementation of the `any` component](http://www.two-sdg.demon.co.uk/curbralan/papers/ValuedConversions.pdf), which led to `boost::function` and `std::function` and the "mainstreaming" of type erasure as an alternative for polymorphism.

The generally accepted jargon for doing polymorphism (implementing sub-typing relationships) without inheritance and `virtual` overrides of member functions is "type erasure".  Within "type erasure", educator Arthur O'Dwyer has coined the concept of "affordance" to refer to the polymorphic operations a "type erased" object is capable of doing, described in his excellent blog article ["What is Type Erasure?"](https://quuxplusone.github.io/blog/2019/03/18/what-is-type-erasure/).

Type erasure is a very active topic of conceptual development in C++, O'Dwyer reports in ["The space of design choices for `std::function`"](https://quuxplusone.github.io/blog/2019/03/27/design-space-for-std-function/) a large number of proposals around merely `std::function` that are under consideration for addition to the language.

The type erasure framework `AnyContainer` in this repository already allowed the straightforward codification of **all of those proposals**, for example, `AnyCallable` indeed is just an straightforward extension of `AnyCallable` and is capable of replacing `std::function` in codebases as large as Snapchat's; the performance, binary size improvements of replacing `std::function` by `AnyCallable` have been hard to believe, yet measured rigorously.  Furthermore, move-only containers were implemented, and derived components or extensions such as move only callables were made covariant with respect to copyability maintaining the performance, binary size and clarity improvements, but we now deem that previous work as superceded by the techniques in the latest framework.

The latest work on this framework, [`GenericPolicy`](https://github.sc-corp.net/emadrid/szr/blob/6e436e6aaf1d12f2d0992bb3e3f9acf9adec289a/test/inc/zoo/Any/VTablePolicy.h#L204) comes to dramatically simplify the process of developing affordances and defining containers with arbitrary sets of affordances, in which they are specified as trivially as using them as template arguments, the code
```c++
    using Policy = zoo::Policy<zoo::Destroy, zoo::Move, zoo::Copy>;
    using VTA = zoo::AnyContainer<Policy>;
```
indicates a type erasure policy that can do destruction, moving and copying.  In the same way, the user may remove `Copy` to have move-only, or add `RTTI` for normal RTTI, or remove destruction for things that are never destroyed during the lifetime of the process (thus we don't add the binary size of the code to destroy them), implement serialization/deserialization affordances, introspection; the user gains granularity with regards to the mechanisms to hold the type erased objects (for example, on memory arenas based on their types for better memory locality) without having to rewrite massive amounts of error-prone and subtle code.

The objective has been to fully preserve the benefits of type erasure, including performance which is the hardest to keep while doing radical extensions, while simplying use, consistent with the overall doctrine in this repository of encoding profound knowledge into source code that then *gives* performance and clarity.

The affordances of moving and especially copying are probably in the hardest of tiers of affordances to implement because they are fundamental operations of the types, thus, they may have the most complicated *implicit API* with the containers and are a good illustration of the maximum challenge users will be faced when doing their own affordances.

