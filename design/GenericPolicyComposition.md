The core mechanism of `AnyContainer` is having the policy tell it the `MemoryLayout` of the local storage to do everything.  The `MemoryLayout` also provides the "micro API".  Without policy compositions the affordances (including user affordances) can change the `MemoryLayout` via the CRTP.

However, this is not possible with composed policies, below is the explanation.

Another key is that the actual objects are managed by different *implementations* based on the properties of the object to be handled: For example, in the current state of the framework, small enough, with compatible alignment and `noexcept` move constructor, they will reside locally, otherwise, through the heap; there are these two families of implementations, the local buffer and the heap.  There are more ways, for example memory arenas for instances of the same type, so the possibilities for implementations on how to manage the objects are not bounded.

At the initial design of `AnyContainer` we chose that the implementations would be *substitutable* by the `MemoryLayout`, that is, we committed to an inheritance relationship.

To achieve substitutability the language puts us in a corner that basically only leaves the tool of converting pointer-to-base-class into pointer-to-derived-class [this is the ultimate direction in which the language is evolving to](http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#2051); and this is the hardest complication of all of the framework:

**The implementations must inherit from `MemoryLayout`**.

Therefore, if we want to substitute among `AnyContainer<P1>` and `AnyContainer<P2>`, we can only do it if `P1::MemoryLayout` is a base class of `P2::MemoryLayout`, and at the same time, to be valid, the implementations in P2 must inherit from the implementations in P1, taken together these two things force that `P1::MemoryLayout` is the same as `P2::MemoryLayout`, and then the extra affordances in `P2` are not allowed to change the `MemoryLayout` via the CRTP.  In essence, **the only way to achieve policy composition is when the `MemoryLayout` is polymorphic without changing its C++ type**.  In the case of vtable-based policies, this is achieved by extending the vtables the `MemoryLayout` points to with the extra affordances, again, using an inheritance relationship.

However, the pre-existing way for `AnyContainer` to reach into `MemoryLayout` for the things the affordances need does not work for the extra affordances in `P2`.  All `AnyContainer` "knows" is its `MemoryLayout` and `P2`.

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


