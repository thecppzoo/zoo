### Introduction to substitutability

Consider how effortlessly we use **containers** and **iterators** from the Standard Template Library (STL) to traverse collections of data—whether it’s a `std::vector`, `std::set`, or `std::unordered_set`, despite them being very different containers, we can use the **same iteration logic without concerning ourselves with the details** of the containers and their iterators.  This is "polymorphism", working with things that have very different details through an interface that represents an abstraction that allows us to accomplish useful things.  In the case of iteration, because it is the compiler the thing that resolves the abstract interface to the details of the specific types, we call this "compile-time" polymorphism.

This abstract interface principle that relieves us from having to know the details of the specific objects is also very valuable at *runtime*, especially if our application is going to produce objects that **we don't know what they are** before running the program.  Other programming languages don't have as good compile-time polymorphism as we have in C++, thus, they focus exclusively on runtime polymorphism, to the point that for most of the Software Engineering community "polymorphism" simply means "runtime polymorphism".  Furthermore, for them, polymorphism and **subclassing** are the same thing: the only practical way to get (runtime) polymorphism is via inheritance, and what we call "`virtual` overrides".  This is the fundamental mechanism of abstraction in those languages, Object Oriented Interfaces.  It turns out that in C++ we can still do that, or the option of doing MUCH BETTER.  In the same way that we can achieve ease of programming with containers and iterators that are not related by inheritance, while at the same time getting optimal or nearly optimal performance, for vast practical cases we can devise runtime polymorphism with the same ease of programming and far superior performance and efficiency —much better than through subclassing.  This is what this presentation is about, ultimately, both working less and achieving objectively better performance.

It is not even difficult per se, you'll see in the solutions we have already coded that they are not that complicated, however, what we must do is to ground our work on **fundamentally superior principles** so that our path to do better is revealed.  Let us not be afraid to dive into the theory.

What we aim to achieve is true **substitutability**, as defined by the Liskov Substitution Principle—allowing seamless replacement of types without compromising correctness or efficiency:

### Liskov's substitution, subtypes

**Substitutability**, as defined by the Liskov Substitution Principle (LSP), is the idea that objects of a given more abstract type, let's say, a "container", should be replaceable with objects of a **subtype**, a more specific or concrete type, let's say a `std::vector`, without altering the correctness of the program. In its essence, it ensures that any code working with an abstraction should not need to know the specific details of the underlying concrete implementation—it should "just work" regardless of the actual type used.  Substitutability implies a relation among types: The interface is the supertype and the implementation is the **subtype**.

Of course that subtypes can themselves be supertypes for further refinement.  Notice that this relation is hierarchical, that's why it is commonly represented in programming code as inheritance.  However, the same types can be subtypes of many subtyping relations, this is one place where inheritance, that tends to work well for single base classes only, creates obstacles to model the fact that the type of an object is a subtype of different subtyping relations.  For example, `std::vector` and `std::map` both map a key to a value (in the case of vectors, the type of key is always an index type), so, they are examples of "mapping containers", however, `std::vector` is an example of a container that stores its data contiguously in memory, while `std::map` store its data as a tree.  So, code that relates to contiguous memory can substitute in a `std::vector`.  We don't think of the many supertypes that something such as `vector` might have because we don't have to make explicit all their subtypes, but when we're using *subclassing*, we must.  That's a huge difference: polymorphism without subclassing admits subtyping relations that are **implicit**.  In particular, supports subtyping relations that are invented or identified **after** the subtypes have been written.  In that regard, supporting polymorphism without subclassing leads to both higher modeling power and less work.

Now, here’s the exciting part: we can translate powerful compile-time abstraction mechanisms, specific to C++, into the runtime domain.  For example, we will see how relatively easy it is to produce types with both "value semantics" and runtime polymorphism, something hugely popular languages such as Java and C# can't do at all.

### Summary

In essence, this presentation is the following:

1. Runtime Polymorphism through subclassing totally sucks.
2. External Polymorphism is how to get (runtime) polymorphism without subclassing: translating compile time polymorphism to runtime.
3. And, if you also want that your "polymorphism without subclassing" would manage the objects, their lifetime, i.e., "*owns*" them, then you probably want "Type Erasure".

### Potential

There's a big problem with External Polymorphism (EP) and Type Erasure (TE): Components like `std::any` and `std::function`, as well as most of the implementations in big foundational libraries such as Facebook's Folly, keep making the same mistakes from 25 years ago, leading to very misleading reputation about both EP and TE, and their true potential completely eclipsed.



