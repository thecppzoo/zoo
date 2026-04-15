# The Alias Method

## Summary
We describe the method for sampling from a discrete probability distribution within the context of Genetic Algorithms and the Multi-Armed Bandit.

### What is the need being fulfilled?

In both Genetic Algorithms and the Multi-Armed Bandit scenarios we want to give more "trials" to the solutions that so far seem better, and fewer trials to the ones that have looked inferior.
Let's say that we have made it so that our preferences are expressed as *probabilities*: We will be selecting randomly which individual to pick for the purposes of recombination, or the "arm" in the multi-armed bandit scenario, in **proportion to the probability of that selection**.
For example, we have a Genetic Algorithm that currently has a population of 4 individuals, and we have these probabilities: A:0.2, B:0.1, C:0.4, D:0.3.
How do we make it so that our random selection ends up reflecting those probabilities, like, for example, that if we make 100 random selections, we would expect to have selected D about 30 times?

#### Naive approach
One choice we have is to make an array of the cumulative probability of the elements in the array, in the example, A:0.2, B:0.3, C:0.7, D:1.0, and our selection could be drawing one element from the range 0 to 1, and then, we find our random draw `r` in the cumulative distribution, for example, using binary search.  Let's say that we randomly get 0.55, that would be higher than the limit for B, of 0.3, but smaller than the limit for C, 0.7, then we would have selected C.  Second example: 0.25: higher than A, less than B, so, we have selected B.
Notice that even with binary search, optimized or not using the "Eytzinger" cache friendly technique that lookup, for each selection, takes O(log(N)) operations.
If our population would consist, let's say, in a million individuals, we would still need to perform around 20 binary search steps to arrive to the element being selected.

### The efficient solution
There is a much better way to do this, that is suitable for both Genetic Algorithms and "MAB" applications: The selection could be as cheap as O(1) (constant time), as cheap as indexing two arrays!
That approach is called the [Alias Method @Wikipedia](https://en.wikipedia.org/wiki/Alias_method).
LLMs explain this concept well, to my direct knowledge, ChatGPT 5 of early 2026.

## The Alias Method

In our previous example, we have something like this (think of each letter as one position): we have a target line, in which individual A is selected twice every ten times, ..., D is selected 3 times:

AABCCCCDDD

This is a convenient way to visualize the target distribution: if we sample uniformly among these 10 positions, then `A` is selected 2 times out of 10, `B` 1 time out of 10, `C` 4 times out of 10, and `D` 3 times out of 10.

In the general case, however we cannot rely on such a neat expanded array:
* The probabilities may not be exact multiples of any convenient tick size,
* and making the array fine-grained enough could require arbitrarily many positions.

The Alias Method gives us a compact way to represent that same sampling behavior using only two arrays of size `N`, constant time sampling and linear time construction of the two arrays (tables), and intuition tells us this is as good as it gets: sampling is reduced to
just a couple of array lookups and one comparison, and building the
alias table cannot take less than linear time if we must at least
inspect all input probabilities

The key idea is that if we have N individuals to select from (in our example 4), then, we will only need two arrays of size N for the sampling, and the selection would be a random draw in the range, as floating point, between (0, n) semiopen.  In our example, the first index, 0, could correspond to individual A.  If we don't do anything else, we would be giving A a whole index, which would correspond to 1/4 of all indices available.  Thus, we would be incorrectly giving more chances to A than granted, the opposite would happen for C: It should be selected 0.4 of the time, however, here it would only be selected 0.25.  The trick is that we can split the bucket for A so that of its size of 0.25 (a quarter) of the total size, only 0.2 would actually go for A, and the remainder, in this case 0.05, would **alias**, hence the name, to some other element that needs more than one bucket, in this case, either C or D, let's say that we alias D, so, now, we need to reduce the remaining probability of D to 0.25 to account for the 0.05 part that is taken care of in the bucket for A.

## Code, pseudo or not

These things can be articulated in code:

The algorithm proceeds looking at each possible event (individual) as an
index. It classifies the events into `small` and `large`: the small are
those for which one bucket is too large; the large, those that need more
than one whole bucket.

Then, it picks a pair of a large and a small (if there are both smalls
and larges), and pair them: now, in the index that represents the
small, there would be a part that maps to itself, and the remainder
aliases to the big.

The small is removed, the original large is removed too, but
reintroduced as an adjusted small or big, depending on whether the
reduction of probability turned it small or not.

And the array of aliases will now note an alias for the large. In our
example, the probabilities array at the position for index `A` will be
`0.2`, and the alias array at position `A` will be set to `D`.

Later, when sampling, we would draw a floating point number uniformly
from the semiopen interval `[0, N)`, and extract the integral part for
the index, and the fractional part to determine whether we go to the
alias or not.
