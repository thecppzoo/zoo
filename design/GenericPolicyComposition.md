The core mechanism of `AnyContainer` is the substitutability of `Policy::MemoryLayout` by the implementations.

The infrastructure work done this far supports two families of implementations, although as the needs arise they could be many more, currently there are the family of "value" implementations and "reference" implementations.  If the `MemoryLayout` storage (the so-called small buffer in the "small buffer optimization") has the size and alignments necessary, **and the copy constructor is `noexcept`**, this is the implementation; otherwise, AnyContainer will be implemented as something that contains a pointer to the value managed on the heap.

To achieve substitutability the language puts us in a corner that basically only leaves the tool of converting pointer-to-base-class into pointer-to-derived-class [this is the ultimate direction in which the language is evolving to](http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#2051); and this is the hardest complication of all of the framework:

**The implementations must inherit from `MemoryLayout`**.

Therefore, if we want to substitute among `AnyContainer<P1>` and `AnyContainer<P2>`, we can only do it if `P1::MemoryLayout` is a base class of `P2::MemoryLayout`, and at the same time, to be valid, the implementations in P2 must inherit from the implementations in P1, taken together these two things force that `P1::MemoryLayout` is the same as `P2::MemoryLayout`.

## Covariance of `AnyContainer<BasePolicy>` and `AnyContainer<ExtendedBasePolicy>`

`ExtendedBasePolicy` is a policy composed from `BasePolicy` and its own extra affordances, that is, the result of
`DerivedPolicy<BasePolicy, X, Y, Z>` where `X`, `Y`, `Z` are affordance specifications.  Notice that `BasePolicy` is not a base class of `ExtendedBasePolicy`.

We want the following code to be completely valid:

```c++
AnyContainer<ExtendedBasePolicy> ebp;
AnyContainer<BasePolicy> *bpp = &ebp, &bpr = ebp;
```

The language forces us to make `AnyContainer<ExtendedBasePolicy>` a derived class of `AnyContainer<BasePolicy>`.

Above we have explained the VTables are compatible.


