### SWAR:
SIMD-within-a-register.  A class of techniques for executing single
instruction, multiple data operations on standard general processing registers.
Allows usage of full number of computing bits in general purpose registers and
material speedups of computational loads in a fairly large number of cases.

### Associative Iteration:
Defined in code, [associative_iteration.h](https://github.com/thecppzoo/zoo/blob/8f2e29d48194fb17bbf79688106bd28f44f7e11c/inc/zoo/swar/associative_iteration.h#L366-L387).
Will be upgraded to a recursive template that changes function arguments and return types in a way in which the new return type is the same as the next recursive invocation argument type.
Associative iteration is a construct that applies associative operators in a way that is advantageous to parallel processing via SWAR techniques.

### Hardware SIMD
Refers to SIMD implemented in actual, real, SIMD architectures such as AVX or ARM Neon.  Implies the direct application of architectural intrinsics, or compiler intrinsics (`__builtin_popcount`) that translate almost directly to assembler instructions.  Note: there are compiler intrinsics such as GCC/Clang's `__builtin_ctz` that would be implemented, in ARM litte endian implementations, as bit reversal followed with counting *leading* zeros: [compiler explorer](https://godbolt.org/z/xsKerbKzK)

### Software SIMD
Refers to the implementation of SIMD using software.  The most prominent example is our SWAR library.  Note that the implementation of SIMD this way is, generally, performant.

### BooleanSWAR:
A construct and convetion used within this repository to hold boolean values.  Stores a 1 or 0 in the most significant bit of a lane to indicate truth.  Remaining bits are zeroed (using the BooleanSWAR object) or dont-care (semantically)

### PsuedoBooleanSWAR:
A construct and convetion used within this repository to hold a singular boolean value.  As used so far (6/24), only the least significant lane marked true is a valid true value. Call this the 'true lane'. All less significant lane than the 'true lane' are zeros.  All more significant lanes than the 'true lane' are unspecified, don't care, or garbage.  Usually used with the lsbIndex() function to find an index for further computation.
