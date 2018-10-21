# zoo

Library of components.

I am accepting adaptation work requests for reasonable fees.

## Dependencies

1. C++ 14 language standard.  We use C++14 for convenience with efforts to make it as straightforward as possible the adaptation to C++ 11, unless complicated `constexpr` functions.
1. Elements of this library depend on [thecppzoo/metatools](https://github.com/thecppzoo/metatools).
2. Tests rely on [Catch2](https://github.com/catchorg/Catch2).
3. These codebases work better with Clang and Libc++ over GCC and libstdc++:  This is not intentional, I don't program to give Clang the advantage, but I program trying to give the compiler as many opportunities to optimize as possible; Clang and libc++ happen to be objectively better.  The documentation and design discussions mention the known differences between compilers and standard libraries.
    1. Bugs of GCC are dealt with in whatever practical way, including non-portable code.  I have great respect for the GCC project and its users but I prioritize development of features over GCC support. However, GCC may generate superior code in some cases.  I am willing to accept paid consulting work to make adaptations to fully support versions of GCC, Intel and Microsoft compilers.

## Repository organization

Adheres to the layout described in ["doctrine"](https://github.com/thecppzoo/thecppzoo.github.io/blob/master/doctrine.md)

## Code style

Adheres to the principles described in ["doctrine"](https://github.com/thecppzoo/thecppzoo.github.io/blob/master/doctrine.md)

## Components

### Algorithms

#### Cache Friendly Search

Inspired by work made by Joaquín M López Muñoz at his blog post ["Cache friendly binary search"](http://bannalia.blogspot.com/2015/06/cache-friendly-binary-search.html), the library provides a [suite of functions](https://github.com/thecppzoo/zoo/blob/master/inc/zoo/algorithm/cfs.h) to convert a random access iterator sorted range to a heap suitable for cache friendly search and the binary search on it.  This component is fully tested and benchmarked.

#### Bidirectional-iterator quicksort

Most implementations of quicksort are not generic and require random access iteration, [here](https://github.com/thecppzoo/zoo/blob/master/inc/zoo/algorithm/quicksort.h) it is provided a generic quicksort implementation that only requires [forward iteration](https://en.cppreference.com/w/cpp/named_req/ForwardIterator).  Especially useful to showcase the advantages of generic programming.

### `AnyContainer`

Currently not documented.

Design discussion [here](https://github.com/thecppzoo/zoo/blob/master/design/AnyContainer.md).

Tested at [any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/any.cpp) and [compilation/any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/compilation/any.cpp)
