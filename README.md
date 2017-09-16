# zoo

Library of components.

I am accepting adaptation work requests for reasonable fees.

## Dependencies

1. C++ 14 language standard.  We use C++14 for convenience with efforts to make it as straightforward as possible the adaptation to C++ 11, unless complicated `constexpr` functions.
1. Elements of this library depend on [thecppzoo/metatools](https://github.com/thecppzoo/metatools).
2. Tests rely on [Catch](https://github.com/philsquared/Catch).
3. These codebases work better with Clang and Libc++ over GCC and libstdc++:  This is not intentional, I don't program to give Clang the advantage, but I program trying to give the compiler as many opportunities to optimize as possible; Clang and libc++ happen to be objectively better.  The documentation and design discussions mention the known differences between compilers and standard libraries.
    1. Bugs of GCC are dealt with in whatever practical way, including non-portable code.  I have great respect for the GCC project and its users but I prioritize development of features over GCC support. However, GCC may generate superior code in some cases.  I am willing to accept paid consulting work to make adaptations to fully support versions of GCC, Intel and Microsoft compilers.

## Repository organization

Adheres to the layout described in ["doctrine"](https://github.com/thecppzoo/thecppzoo.github.io/blob/master/doctrine.md)

## Code style

Adheres to the principles described in ["doctrine"](https://github.com/thecppzoo/thecppzoo.github.io/blob/master/doctrine.md)

## Components

### `AnyContainer`

Currently not documented.

Design discussion [here](https://github.com/thecppzoo/zoo/blob/master/design/AnyContainer.md).

Tested at [any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/any.cpp) and [compilation/any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/compilation/any.cpp)
