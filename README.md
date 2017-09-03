# zoo

Library components.

## Dependencies

1. C++ 14 language standard.  We use C++14 for convenience with efforts to make it as straight forward as possible the adaptation to C++ 11, unless complicated `constexpr` functions.
1. Elements of this library depend on [thecppzoo/metatools](https://github.com/thecppzoo/metatools).
2. Tests rely on [Catch](https://github.com/philsquared/Catch).
3. These codebases work better with Clang and Libc++ over GCC and libstdc++:  This is not intentional, I don't program to give Clang the advantage, but I program trying to give the compiler as many opportunities to optimize as possible; Clang and libc++ happen to be objectively better.  The documentation and design discussions mention the known differences between compilers and standard libraries.
    1. Bugs of GCC are dealt with in whatever practical way, including non-portable code.  I have great respect for the GCC project but I prioritize development of features over GCC support because Clang tends to be clearly superior.  However, GCC may generate superior code in some cases.

## Common Organization of Repositories

1. <repository>/README.md (such as this file) should be the user documentation entry point.  It is a documentation bug if documentation is not accessible, referenced in the natural way starting from this file.  Please file the bug if documentation exists but not linked from here
1. Include files have their root at <repository>/inc
2. `.cpp` files with non-inline implementations of library components at <repository>/lib/<component>
3. Tests at <repository>/test, one single `.cpp` file per group of tests
    1. Tests may have a subdirectory `<repository>/test/compilation` for compilation tests of programs that need to be malformed.  These tests are bracketed by #ifdef <name of test> ... #endif, see the example at [`.../test/compilation/any.cpp`](https://github.com/thecppzoo/zoo/blob/63c74903abc61ce78d71c5ece843fbcd867d4a68/test/compilation/any.cpp#L13)
4. Source files for applications, demoes at <repository>/src
5. <repository>/design for design documents

## Notes

1. Wrong comments in the source code are considered a bug as serious as broken functionality, please report
2. Poor choices of identifiers, that lead to otherwise unnecessary comments or explanations are considered a design error.  Please report, the sooner these errors are corrected, the easier to correct them and the less harm they cause in the projects that use the poor identifiers.
3. Unnecessary comments in the source code are a clarity bug, reporting appreciated.
4. I am poor on encapsulating public interfaces because of practical reasons:  Tactical opportunities to test internal mechanisms and because internal mechanisms are an evolving understanding of the subject matter.  Encapsulation is the result of source code maturity
5. Unnecessary explicit typing (using data types explicitly when they can be deduced automatically) is considered a latent functionality bug, because if the code is good, sooner than later it will be upgraded to a template and then explicit types become dangerous.  If the reader of source code requires the help of the redundant explicit typing to understand what is the code doing is considered to be an indication that the code is confusing.

In my doctrine of software engineering if the programmer can't write clear code they won't write worthy comments or documentation nor design documents (this precept was taken from Kevlin Henney).  Documentation, design documents and source code should mirror each other, each should express the best knowledge available about the subject matter.  Source code can double as documentation or design.

## Components

### `AnyContainer`

Currently not documented.

Design discussion [here](https://github.com/thecppzoo/zoo/blob/master/design/AnyContainer.md).

Tested at [any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/any.cpp) and [compilation/any.cpp](https://github.com/thecppzoo/zoo/blob/master/test/compilation/any.cpp)
