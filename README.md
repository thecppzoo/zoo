# zoo

Library of components.

I am accepting adaptation work requests for reasonable fees.

## Dependencies

1. C++ 14 language standard.  We use C++14 for convenience with efforts to make it as straightforward as possible the adaptation to C++ 11, unless complicated `constexpr` functions.
1. Elements of this library depend on [thecppzoo/metatools](https://github.com/thecppzoo/metatools).
2. Tests rely on [Catch](https://github.com/philsquared/Catch).
3. These codebases work better with Clang and Libc++ over GCC and libstdc++:  This is not intentional, I don't program to give Clang the advantage, but I program trying to give the compiler as many opportunities to optimize as possible; Clang and libc++ happen to be objectively better.  The documentation and design discussions mention the known differences between compilers and standard libraries.
    1. Bugs of GCC are dealt with in whatever practical way, including non-portable code.  I have great respect for the GCC project and its users but I prioritize development of features over GCC support. However, GCC may generate superior code in some cases.  I am willing to accept paid consulting work to make adaptations to fully support versions of GCC, Intel and Microsoft compilers.

## Common Organization of Repositories

1. \<repository>/README.md (such as this file) should be the user documentation entry point.  It is a documentation bug if documentation is not accessible, referenced in the natural way starting from this file.  Please file the bug if documentation exists but not linked from here
1. Include files have their root at \<repository>/inc
2. `.cpp` files with non-inline implementations of library components at \<repository>/lib/<component>
3. Tests at \<repository>/test, one single `.cpp` file per group of tests
    1. Tests may have a subdirectory \<repository>/test/compilation for compilation tests the specifications demand they must be malformed programs (for example, trying to build a non-copy-constructible value in an `any` component).  These tests are bracketed by #ifdef <name of test> ... #endif, see the example at [`.../test/compilation/any.cpp`](https://github.com/thecppzoo/zoo/blob/63c74903abc61ce78d71c5ece843fbcd867d4a68/test/compilation/any.cpp#L13)
4. Source files for applications, demoes at \<repository>/src
5. \<repository>/design for design documents

## Doctrinary notes

1. Wrong comments in the source code are considered a bug as serious as broken functionality, please report
2. Poor choices of identifiers, that lead to otherwise unnecessary comments or explanations are considered a design error.  Please report, the sooner these errors are corrected, the easier to correct them and the less harm they cause in the projects that use the poor identifiers.
3. Unnecessary comments in the source code are a clarity bug, reporting appreciated.
4. I am poor about encapsulating public interfaces because of practical reasons:  All public presents tactical opportunities to test internal mechanisms and because internal mechanisms are an evolving understanding of the subject matter.  Encapsulation into public/private interfaces is the result of source code maturity that should not be rushed but respected to arise organically.
5. Unnecessary explicit typing (using data types explicitly when they can be deduced automatically) is considered a latent functionality bug:
    1. If the code is good, sooner than later it will be upgraded to a template and then explicit types become dangerous.
    2. If the reader of source code requires the help of redundant explicit typing to understand what is the code doing it is considered to be an indication that the code is confusing.
    3. Redundant explicit typing provide an incentive for the code to commit to the specific features of the explicit type, increasing the work to make the code generic.
6. Tests of public interfaces should double as user manuals.
7. If the programmer can't write clear code they won't write worthy comments or documentation nor design documents (this precept was taken from Kevlin Henney).  Documentation, design documents and source code should mirror each other, each should express the best knowledge available about the subject matter.  Source code can double as documentation or design.
8. I work looping (spiralling?) between making tests, programming, writing design documents, and perhaps documenting.  The sequence of commits illustrates the improvements in understanding of the subject matter including how writing design leads to better code.

## Components

### `AnyContainer`

Currently not documented.

Design discussion [here](https://github.com/thecppzoo/zoo/blob/master/design/AnyContainer.md).

Tested at [any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/any.cpp) and [compilation/any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/compilation/any.cpp)
