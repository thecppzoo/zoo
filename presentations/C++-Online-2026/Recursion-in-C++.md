# Recursive Functions in C++

There are many programming scenarios in which self-similarity, as for example evaluating programs in a Genetic Programming engine, in which programs are essentially trees, can be mission critical and strongly suggest code that uses *recursion* (a function calling itself).

However, this is not well supported in C, and C++ inherits the problem.

First, the way to get the best performance out of recursion is to actually resolve the recurrence, to transform the recursion into iteration with an explicit stack.  Even in 2026, a competent programmer may succeed in practice: the amount of work needed to avoid two distinct deficiencies of recursion may be reasonable:

1. The recursive function must respect the calling convention in the ABI: it is a one-size-fits-all specification that leads to typical "Goldilocks" issues: reserve too large amounts of stack space, save too many registers, or too few; this tends to be much worse than ills of the equivalent explicit stack.
2. Recursion, in this era, almost always becomes a compiler optimizer barrier: the optimizers are "disempowered".

One example is my implementation of `quicksort`: it just needs a stack in which each frame is simply the bounds of the range, and I can get away with having a "local" array of 64 elements, under the assumption that no input will have more than 2^64 elements, and the code "recurs" in a way that it guarantees that the maximum "recursion" depth would be less than log(input): "recur" only on the shorter quicksort partition, reuse the current stack frame for the larger.

Then, the question is not whether this is a practical approach, but that there is a nasty problem here, in C++ inherited from C:

A recursive function may be called recursively from more than one place.

For ordinary recursion, the compiler lowers the continuation (what is going to be executed upon return) quite easily and effectively: push the continuation to the stack (the return address), and this is the same thing used for all function calls, so, everything from the ABI to the processor core's stream of execution resources optimize for it.

From portable C (and thus C++) you can't treat a continuation as a value, thus you can't put it in your explicit stack.  What we do to overcome this language design failure is to represent the continuation as a value of an enumerated type, and put that in the stack frame.  That is:

`enum Continuation { ReturnToCaseA, ReturnToCaseB, ReturnToCaseC };`

And then, when that frame is popped, execution resumes by switching on
the popped enumerated value:

```
switch (frame.continuation) {
    case ReturnToCaseA:
        /* resume A */
        break;
    case ReturnToCaseB:
        /* come back to B */
        break;
    case ReturnToCaseC:
        /* complete C */
        break;
}
```

What would be wrong with that?
That these `switch`es lower to an indirect jumps, that are significantly less performant than jumping to the return address on the current stack frame.
Furthermore, indirect jumps, in this era, are also optimizer barriers.

Except for scenarios in which this recursion problem of continuing from different places can be addressed with ad-hoc or exotic idioms, we are back to the same type of bad performance but for error prone work.

Notice something important: Despite quicksort typically being taught as a function that recurs in both partitions, you can see that it can be seen as iterating on one ever-shrinking partition, let's say, the "smaller than pivot partition", and recurring on the other, so, it does not exhibit the multiple recursive continuation problem.  My implementation "recurs" (the ironic quotes are to denote that it uses the stack machinery) on the partition with the fewest elements, that's why it works.  The iterative formulation of the Santa Fe Artificial Ant genome evaluation in my Genetic Programming example was much harder: it would normally fully exhibit this "multiple continuation problem", but this particular domain of Genetic Programming has the ad-hoc property that all of the non-primitives (Prog-n and if-food-ahead) execute a specific count of descendant subtrees without interdependence between them, therefore, all recursive returns could be simply "execute my sibling in the subtree", which is data that can be directly represented in the stack ("who's my sibling").  But it took "pulling teeth" to even realize this was the ad-hoc way to sidestep the multiple continuations problem; furthermore, this continues to be totally ad-hoc for **this particular domain**!

A coroutine, in principle, may help because it already represents a
suspended computation together with its resumable continuation as ad-hoc function call conventions: devised by the optimizer for the particular needs, not the ABI's "one size fits all".

That does not mean there is already a mainstream solution. It means that
the object code parsimony of coroutine ad-hoc calling conventions may point toward a
better way of handling this problem, a topic I am currently researching.
