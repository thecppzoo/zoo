The Floyd sampling algorithm --you can see an excellent exposition [here](http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html)-- is very convenient for use cases such as getting a hand of cards from a deck.

The fastest way to represent sets of finite and small domains (such as a deck of cards) seems to be as bits in bitfields.

For an example of a deck of 52 cards, we may want, for example, to generate all of the 7-card hands.  I wrote an straightforward implementation [here](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/Floyd.h).  Its interface is this:

```c++
template<int N, int K, typename Rng>
inline uint64_t floydSample(Rng &&g);
```

With speed in mind, the size of the set and subset are template parameters.  Compilers such as Clang, GCC routinely generate optimal code for the given sizes, as can be seen in the compiler explorer, which means they take advantage of those parameters being template parameters.  The return value is the subset expressed as the bits set in the least significant N bits of the resulting integer.

However, what if the use case is to generate a sample (subset) of the *remaining* members of the set? for example, to generate a random 2-card *after* five cards have been selected?

That has been implemented too, in a function with this signature:

```c++
template<int N, int K, typename Rng>
inline uint64_t floydSample(Rng &&g, uint64_t preselected)
```

Here, `preselected` represents the cards already selected.  If what is desired is to get two cards from the cards remaining after selecting `fiveAlreadySelected` cards, the call `ep::floydSample<47, 2>(randomGenerator, fiveAlreadySelected)` will suffice.  Notice the template argument for `N` is now 47, reflecting the fact that the remaining set of cards has 47 cards.  Unfortunately, it is difficult to guarantee at compilation time that the argument `fiveAlreadySelected` indeed has exactly five elements, because operations such as intersection or union result in sets with cardinalities that are fundamentally run-time values.

This overload for `ep::floydSample` requires calling a "deposit" operation.  This is an interesting operation hard to implement without direct support from the processor:  Given a mask, the bits of the input will be "deposited" one at a time into the bit positions indicated as bits set in the mask.  In the AMD64/Intel architecture EM64T this is supported in the instruction set "BMI2" as the instruction [`PDEP`](https://chessprogramming.wikispaces.com/BMI2).  The implementation of the adaptation of the Floyd algorithm for a known number of preselected elements is then straightforward: discount from the total the number of bits set, call normal floydSample, and "deposit" the result in the inverse of the preselection.

What are the costs of these implementations?

1. The programmer needs to indicate at compilation time the number of elements in the set.  If this number is a runtime value, a `switch` will be needed to convert runtime to compile time numbers, that transforms into an indexed jump at the assembler level.
2. All of the operations in the normal Floyd sampling algorithm are negligible in terms of execution costs compared to calling the random number generator, which is essential in each iteration.
3. The adaptation to account for preselections only requires two assembler instructions more: inverting the preselection and depositing it.  `PDEP` has been measured to be an instruction with a throughput of one per clock, which is excellent compared to implementing it in software; however, in current processors it can only be executed in a particular pipeline.  In Pokerbotic we don't think we are oversubscribing this pipeline, so we suspect we get a 1-per-clock throughput for this use case.
4. However, the adaptation to account for preselections also require the programmer to accurately indicate the cardinality of the preselection.  This can add the same cost as number 1 here, plus the population count, another single-pipeline, 1-per-clock throughput instruction.

We are interested in any way to implement a faster subset sample selection.  This use case is at the heart of many operations in Pokerbotic.

