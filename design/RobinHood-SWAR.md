# Design notes concerning Robin Hood implemented with SWAR techniques.

## TL;DR

Here are some annotations about design, implementation choices that are better
described in natural text than alongside the source code.
This is not complete.

## Some notes

The types in the "core" routines use return types with multiple values but they
don't introduce performance problems, on the contrary, these calculated values
are made available if needed afterward and prevent recomputation or allow a
simpler programming style.

I (Eddie) have seen in the generated assembler that the return values of type
struct of multiple fields do not trigger a performance penalty, the reason is
that all of the important routines are inlined, the compiler merges all of the
C++ sources to be inlined and thus trivially eliminate the things that are not
needed in a particular path.
