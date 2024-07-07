### Associative Iteration:
Defined in code, [associative_iteration.h](https://github.com/thecppzoo/zoo/blob/8f2e29d48194fb17bbf79688106bd28f44f7e11c/inc/zoo/swar/associative_iteration.h#L366-L387).
Will be upgraded to a recursive template that changes function arguments and return types in a way in which the new return type is the same as the next recursive invocation argument type.
Associative iteration is a construct that applies associative operators in a way that is advantageous to parallel processing via SWAR techniques.

### BooleanSWAR:
A construct and convetion used within this repository to hold boolean values.  Stores a 1 or 0 in the most significant bit of a lane to indicate truth.  Remaining bits are zeroed (using the BooleanSWAR object) or dont-care (semantically)

### Hardware SIMD
Refers to SIMD implemented in actual, real, SIMD Instruction Set Architectures (ISAs) such as AVX or ARM Neon.  Implies the direct application of architectural intrinsics, or compiler intrinsics

### PsuedoBooleanSWAR:
A SIMD Concept we are developing.  As used so far (6/24), only up to the first boolean `true` (from least significant lane upward) are valid, the rest are garbage/don't cares.

### Software SIMD
Refers to the implementation of SIMD using software.  The most prominent example is our SWAR library.

### SWAR:
SIMD-within-a-register.  A class of techniques for executing single instruction, multiple data operations on normal integer datatypes (unsigned types of 32, 64, or 128 bits as supported by the processor).  We are generalizing SWAR to mean not just the "R" of "register" to encompass SIMD-types and operations and eventually also GPU computations.
