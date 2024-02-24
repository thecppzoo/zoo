# Design of the hand classification mechanism in Pokerbotic

## Detection of N-of-a-kind

Detection of N-of-a-kind, two pairs and full house is described [here](https://github.com/thecppzoo/pokerbotic/blob/master/design/What-is-noak-or-how-to-determine-pairs.md).

## Detection of flush

Flush detection happens at [Poker.h:78](https://github.com/thecppzoo/pokerbotic/blob/master/inc/ep/Poker.h#L78) the hand is filtered per each suit, and the built in for population count on the filtered set is called.  This code assumes a hand can only have one suit in flush (or that the hand has up to 9 cards).

## Detection of straights

The straightforward way to detect straights, if the ranks would be consecutive bits (which we will call "packed representation") is this:

```c++
unsigned straights(unsigned cards) {
    auto shifted1 = cards << 1;
    auto shifted2 = cards << 2;
    auto shifted3 = cards << 3;
    auto shifted4 = cards << 4;
    return cards & shifted1 & shifted2 & shifted3 & shifted4
}
```

By shifting and doing a conjuction at the end, the only bits in the result set to one are those that are succeeded by four consecutive bits set to one.  Before accounting for the aces to work as "ace or one", there are possible improvements to be discussed:

### Checking for the presence of 5 or 10

In a deck of 13 ranks starting with 2, all straights must have either the rank 5 or the rank ten.  This has a probability of nearly a third; however, testing for this explicitly is performance disadvantageous.  It seems the branch is fundamentaly not predictable by the processor, so, the penalty of misprediction overcompensates the benefit of early exit.  In the code above, there are 8 binary operators and 4 compile-time constants, there is little budget for branch misprediction.  Older versions of the code had this check until it was benchmarked to be a disadvantage.

### Checking for partial conjunctions

For the same reason, testing if any of the conjunctions is zero to return 0 early is not performance advantageous, confirmed through benchmarking.

### Addition chain

There is one improvement that benchmarks confirm:

```c++
unsigned straights(unsigned cards) {
    // assume the point of view from the bit position for tens.
    auto shift1 = cards >> 1;
    // in shift1 the bit for the rank ten now contains the bit for jacks
    auto tj = cards & shift1;
    auto shift2 = tj >> 2;
    // in shift2, the position for the rank ten now contains the conjunction of originals queen and king
    auto tjqk = tj & shift2;
    return tjqk & (cards >> 4);
}
```

This implementation (which does not take into account the ace duality) requires 6 binary operations and 3 constants and accomplishes the same thing as the straightforward implementation.  Benchmarks confirm this taking roughly 3/4 of the time than the straightforward implementation.

The key insight here is to view the detection of the straight as adding up to 5 starting with 1.  The straightforward implementation does the equivalent of `1 + 1 + 1 + 1 + 1`, this new implementation does `auto two = 1 + 1; return two + two + 1`.  This technique is to build an *addition chain*.  This technique was inspired by the second chapter of the book ["From Mathematics To Generic Programming"](https://www.amazon.com/Mathematics-Generic-Programming-Alexander-Stepanov/dp/0321942043)

Taking into account the dual rank of aces is simply to turn on the 'ones' if there is the ace, but this requires left shift to make room for it.  This can be done at the beginning of the straight check, and its cost can be amortized by the compiler doing a conditional move early, meaning the result will be ready by the time it is used.

There is one further complication in the code, which is that the engine uses the rank-array representation.  Provided that the shifts are for 4, 8, 12, 16 bits instead of 1, 2, 3, 4 there isn't yet a difference.  There are two needs for straights:

1. Normal straights, in which the suit of the rank does not matter.  This is accomplished by making the 13 rank counts as described in how to detect pairs, etc., and using the SWAR operation `greaterEqual<1>` prior to the straight code.  Naturally, the straights don't incurr in an extra cost of doing popcounts because they are amortized in the necessary part of detection of pairs, three of a kind, etc., the `greaterEqual<N>(arg)` operation requires two constants and two or three assembler operations, depending on how the result is used, thus, for practical purposes have negligible cost compared to a packed rank representation.
2. Straights to detect straight flush:  Since the bits for

We suspect our detection of straight code is maximal in terms of performance.
