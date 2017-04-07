# What the hell does "noak" mean in Pokerbotic? or how did you implement detection of pairs, three of a kind, four of a kind?

"Noak" in Pokerbotic code means N-Of-A-Kind, where N is 2, 3, or 4.

In Pokerbotic, more than a full third of the execution time of classification of hands is spent in determining whether the hand is a four-of-a-kind, a full-house, three-of-a-kind, a pair, or just a high-card.  The other roughly two thirds are spent at about a similar execution time cost by the detection of straights and detection of flushes.

Compared to other poker engines, such as Poker Stove, our implementation is easier to use and as fast (around 260 million evaluations per second in popular hardware today).

Pokerbotic represents the cards as bits in an integer, in which the four least significant bits are, from least significant, the 2 of spades, `s2`, hearts, `h2`, diamonds and clubs; then the 3 of spades, and so on up to the ace of clubs, `cA`.  Nominally, each nibble represents the four different suits of the number.  This is implicit in the definition of the enumerations at [`ep::abbreviations::Cards`](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/PokerTypes.h#L64) at [`PokerTypes.h`](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/PokerTypes.h)

## Mechanisms details

Determining the number of repetitions of a number/rank in a hand then corresponds to determining the number of bits set to one in each of the positions of the array.  Counting the number of bits set to one is called "population count" or "hamming weight".  The AMD64/EM64T architecture has the `POPCNT` instruction since SSE4.2, however, we were not able to figure out an advantageous way to use `POPCNT` to find the counts for each of the 13 numbers in a normal deck of cards.  Instead I came up with a [SWAR mechanism](https://en.wikipedia.org/wiki/SWAR) to accomplish these things:

1. Getting all 13 population counts
2. Determining all the rank-array positions that repeat more than N times, where N is a compilation time value
3. Determining the highest rank that satisfies a condition

To get the 13 population counts the code follows the first stages of the most popular way to find the population count when there is no provided built-in:

```c++
int popcntNibbles(int v) {
    constexpr auto mask0 = 0x5...5;
    v -= (v >> 1) & mask0;
    constexpr auto mask1 = 0x3...3;
    v = (v & mask1) + ((v >> 2) & mask1);
    return v;
}
```

Our code in [`SWAR.h`](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/core/SWAR.h) does not look like that at all, but that is what it does.  The implementation is split into several useful components:

1. A bit mask generator
2. A template that based off the SWAR size determines whether to use the assembler instruction `POPCNT` or the logic, or potentially table lookups.
3. The SWAR register code

These components hurt readability, but are essential to fine tune the most performance sensitive operations in Pokerbotic.  As a matter of fact, the current configuration, in which the population count of a SWAR of 16 bit or larger elements occurs calling the `POPCNT` instruction four times, and for a SWAR of 8 bit or smaller elements is through the logic, was determined by benchmarking.  The benchmarking also proved table lookups (not present in the code) to not be advantageous compared to the best between the assembler instruction or the logic at any SWAR size.  This configuration has been proven optimal for current AMD64/EM64T architecture processors, but of course, other processors may have different optimal configurations.  To come up with the optimal configuration, we have:

1. Very fine grained choices on the parameters
2. The code to test things together

## SWAR register user interface

The template [`ep::core::SWAR`](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/core/SWAR.h#L112) takes two parameters: the size of each individual data element and the containing register, for a rank-array representation of a normal deck of cards, 4 and `uint64_t`.

Its public interface member functions are straightforward, perhaps with the exception of `top()` and `fastIndex()`.  They both return an index to a non-null data element.  For example, if a SWAR operation determines the elements whose values are above 2, like for example, a SWAR of rank-counts when determining three-of-a-kinds, the code is interested in finding the highest rank repeated more than 2 times.  This is accomplished calling `top()`.

Unfortunately, the counting of leading zeroes must be converted to a bit index by substraction from 63, thus in general it is less performing than finding the lowest array element.  That is the reason for `fastIndex()`, which returns the lowest non-null index.  This is useful whenever the code cares about getting the index of any non-null data element.  Furthermore, in AMD64/EM64T there are two ways in which the GCC builtin gets translated to, to `BSR` or `LZCNT` that behave differently and have subtle performance differences depending on how they are called.

When implementing the deck representation we found that higher-rank is higher-bit-index overperforms the representation of higher-rank-is-lower-bit-index, because even if all of the rank comparison operations are interested in the highest rank and in SWAR registers they are less performing as higher-rank-more-significant-bit, straights, flushes, and sets of cards in general can be compared much, much faster if the highest rank is at the most significant bits using plain unsigned integer comparisons.

There is an important free standing template function, [`greaterEqualSWAR`](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/core/SWAR.h#L161), that given a SWAR and an integer template parameter N will return a boolean SWAR with the data elements whose values are greater than or equal to the given N.

## Comparisons to other choices

Poker Stove uses table lookups for almost all of its combinatorial logic.  We are very surprised that the logic approach we followed leads to the same performance with the benchmark granularity we've tested, because they are very different.

We still strongly prefer the logic-based approach, because it is branch-less code with no dependencies on the input data, in terms of performance it will behave the same way all the time.  However, the table-lookup based approach looks better in benchmark code than in real life, because it requires the caches to have the tables in cache memory; while it is complicated to generate realistic benchmarks in which on purpose the table-lookup mechanism is starved of cache memory for the tables, this may happen in real life.  Especially with random data that leads to less predictable memory access patterns, and in hyperthreading in which several threads compete for the caches within the same core.
