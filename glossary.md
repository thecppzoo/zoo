### Associative Iteration:
Defined in code, [associative_iteration.h](https://github.com/thecppzoo/zoo/blob/8f2e29d48194fb17bbf79688106bd28f44f7e11c/inc/zoo/swar/associative_iteration.h#L366-L387).
Will be upgraded to a recursive template that changes function arguments and return types in a way in which the new return type is the same as the next recursive invocation argument type.

### Hardware SIMD
Refers to SIMD implemented in actual, real, SIMD architectures such as AVX or ARM Neon.  Implies the direct application of architectural intrinsics, or compiler intrinsics (`__builtin_popcount`) that translate almost directly to assembler instructions.  Note: there are compiler intrinsics such as GCC/Clang's `__builtin_ctz` that would be implemented, in ARM litte endian implementations, as bit reversal followed with counting *leading* zeros: [compiler explorer](https://godbolt.org/z/xsKerbKzK)

### Software SIMD
Refers to the emulation of SIMD using software.  The most prominent example is our SWAR library.
