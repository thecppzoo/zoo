# Latency within the context of SWAR discussions

Recently a colleague asked me about the latencies in applications of our SWAR library.

Are they high?

## Deep data dependencies

Let's see, for example, what Prof. Daniel Lemire said of one of my favorite pieces of his code, "parsing eight digits", discussed in his blog [Quickly parsing eight digits](https://lemire.me/blog/2018/10/03/quickly-parsing-eight-digits/):
```c++
uint32_t parse_eight_digits_swar(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, 8);
  val = val - 0x3030303030303030;
  uint64_t byte10plus   = ((val        
      * (1 + (0xa  <<  8))) >>  8)
      & 0x00FF00FF00FF00FF;
  uint64_t short100plus = ((byte10plus 
       * (1 + (0x64 << 16))) >> 16) 
       & 0x0000FFFF0000FFFF;
  short100plus *= (1 + (10000ULL << 32));
  return short100plus >> 32;
}
```

Lemire commented:

> The fact that each step depends on the completion of the previous step is less good: it means that the latency is relatively high

Certainly this implementation is a long chain of data dependencies, or deep data dependencies, but I still think Lemire's comment is unfortunate, the main topic of this essay.

By the way, we [implemented this code using SWAR](https://github.com/thecppzoo/zoo/blob/9a1e406c63ede7cbc16cf16bde3731fcf9ee8c86/benchmark/atoi.cpp#L33-L55):
```c++
uint32_t calculateBase10(zoo::swar::SWAR<8, uint64_t> convertedToIntegers) noexcept {
    /* the idea is to perform the following multiplication:
     * NOTE: THE BASE OF THE NUMBERS is 256 (2^8), then 65536 (2^16), 2^32
     * convertedToIntegers is IN BASE 256 the number ABCDEFGH
     * BASE256:   A    B    C    D    E    F    G    H *
     * BASE256                                 10    1 =
     *           --------------------------------------
     * BASE256  1*A  1*B  1*C  1*D  1*E  1*F  1*G  1*H +
     * BASE256 10*B 10*C 10*D 10*E 10*F 10*G 10*H    0
     *           --------------------------------------
     * BASE256 A+10B ....................... G+10H   H
     * See that the odd-digits (base256) contain 10*odd + even
     * Then, we can use base(2^16) digits, and base(2^32) to
     * calculate the conversion for the digits in 100s and 10,000s
     */
    auto by11base256 = convertedToIntegers.multiply(256*10 + 1);
    auto bytePairs = zoo::swar::doublePrecision(by11base256).odd;
    static_assert(std::is_same_v<decltype(bytePairs), zoo::swar::SWAR<16, uint64_t>>);
    auto by101base2to16 = bytePairs.multiply(1 + (100 << 16));
    auto byteQuads = zoo::swar::doublePrecision(by101base2to16).odd;
    auto by10001base2to32 = byteQuads.multiply(1 + (10000ull << 32));
    return uint32_t(by10001base2to32.value() >> 32);
}
```
resulting in identical code generation even with minimal optimization, and we will keep iterating so that the code reflects this is just one example of what we call "associative iteration" --[Glossary](../../glossary.md#associative-iteration)--.

## Parallelism

The essence of parallel algorithms is the "mapping" of an operation that is associative to multiple data elements simultaneously or in parallel, and the collection (reduction, gathering, combination) of the partial results into the final result.

The combination of the partial results induce a tree structure, whose height is in the order of the logarithm of the number of elements N.
For a binary operation, the minimum height for this tree is the logarithm base 2 of the number of initial elements: for a binary operation, the minimum latency is achieved when at each pass we can reduce the data elements by half.

This is what happens in Lemire's method:  There are enough execution resources to halve 8 byte elements into 4.  The second stage reduces 4 partial results into 2.
It is clear that with the mathematical technique of squaring the base, **this mechanism is optimal with regards to latency** compared to anything else that uses this numeric base progression scheme.

A comparison with a fundamentally different technique is to revert to the linear, serial mechanism of "multiplying by 10 the previous result and adding the current digit".  That obviously leads to a latency of 7 partial results, multiplication is present in the stages of both schemes, and multiplication is the dominant latency[^1], 7 is worse than 3, so, Lemire's mechanism has, relatively, much lower, not higher, latency.

Then Lemire is comparing his mechanism to some unknown mechanism that might have smaller latency that can not be the normal mathematics of multiplication by a base.  The mathematical need is to convert numbers in base 10 into base 2.  There may be exotic mathematics for base conversions that use other methods, perhaps something based on fourier transform as the best multiplication algorithms, but for sure would be practical only for immense numbers of digits.

No, Lemire's is, for practical applications, the minimum latency.  At each stage of gathering results, the multiplication is the dominant latency.  Can we devise processing stages of smaller latencies?

I strongly suspect not, since the cheapest instructions such as bitwise operations and additions have latencies in the order of 1 cycle.  This method requires 1 multiplication, 1 mask, and 1 shift per stage, not more than 6 cycles, let's say that all of the register renaming, constant loading take one cycle.  That's 7 cycles per stage, and the number of stages is minimal.  See that multiplying by 10 as an addition chain requires 5 steps, so, that's a dead end.  Even the trick of using "Load Effective Address" in x86-64 using its very complex addressing modes ends quickly.  We can see this in the [compiler explorer](https://godbolt.org/z/PjMoGzbPa), multiplying by 10 might be implemented with just 2 instructions of minimal latency (adding and `LEA`), but the compiler optimizer would not multiply by 100 without using the actual multiplication instruction.

Lemire's point of view that his mechanism has high latency seems to be just wrong.

This example goes to show why SWAR mechanisms also have good latencies:  If the SWAR operation is the parallel mapping of computation and the parallel combination of partial results, they very well be of optimal latency.

## How [hardware SIMD](../../glossary.md#hardware-simd) leads to inefficiences due to design mistakes

Sometimes there is no need to combine partial results, for example, in `strlen`, we don't need to, for example, calculate the number of bytes that are null, we just want the first null; any good SIMD programming interface would give you "horizontal" operations to extract the desired result directly.

Curiously enough, our SWAR library gives that capability directly; in the `strlen` case, the operation is "count trailing zeros", however, most SIMD architectures, what we call "hardware SIMD", do not!

I think our colleagues in GLIBC that are so fond of assembler (we have proven this since our equivalents are more than 90% just C++ with very topical use of intrinsics, but theirs are through and through assembler code) would have identified the architectural primitive that would have spared them to have to use general purpose computation to identify the index into the block where the first null occurs.  Since they need to create a "mask" of booleans, it seems that those horizontal primitives are missing from AVX2 and Arm Neon.

In our Robin Hood implementation there are many examples of our generation of a SIMD of boolean results and the immediate use of those booleans as SIMD values for further computation. I believe this helps our SWAR, ["software SIMD"](https://github.com/thecppzoo/zoo/blob/em/essay-swar-latency.md/glossary.md#software-simd) implementations to be competitive with the software that uses "hardware SIMD".

This also shows a notorious deficiency in the efforts to standardize in C++ the interfaces for hardware SIMD: The results of lane-wise processing (vertical processing) that generate booleans would generate "mask" values of types that are bitfields of the results of comparisons, not SIMD type values.  So, if you were to consume these results in further SIMD processing, you'd need to convert and adjust the bitfields in general purpose registers into SIMD registers, this "back and forth SIMD-scalar" conversions would add further performance penalties that compiler optimizers might not be able to eliminate, for a variety of reasons that are out of the scope of this document, wait for us to write an essay that contains the phrase "cuckoo-bananas".

Our SWAR library, in this regard, is "future proof": boolean results are just an specific form of SIMD values.  We use this already for SIMD operations such as saturated addition: unsigned overflow (carry) is detected lane wise and lane-wise propagated to the whole lane, without conversion to scalar.  Saturation is implemented seamlessly as after-processing of computation, without having to convert to "scalar" (the result mask) and back.

In the early planning stages for support of "hardware" SIMD (architectures that do SIMD), I've thought that perhaps we will have to keep our emulated SIMD implementations on "hardware SIMD" as a flavor for users to choose from, since they may be the only way to avoid performance penalties that are higher than the cost of emulation, to prevent the error in the standard library.

## A hard challenge

As with much of what superficially looks bad in our SWAR library, upon closer examination, we determine fairly rigorously that these are the best choices, the essential costs.  In that regard, it meets the primary mission of serving as a way to express in software all that we know about performance so that users can benefit the most from our expertise.

[^1] We're beginning to work on explaining the basics of processor instructions and operations costs.

-ed

