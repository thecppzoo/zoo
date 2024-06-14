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

By the way, I "trust" so much this approach to changing the base, that I [implemented this code using SWAR](https://github.com/thecppzoo/zoo/blob/9a1e406c63ede7cbc16cf16bde3731fcf9ee8c86/benchmark/atoi.cpp#L33-L55):
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
despite the source code looking so different, it results in identical generated code compared to Lemire's original.

Further, this approach to changing bases is one example of what we call "associative iteration" --[Glossary](../../glossary.md#associative-iteration)--, we are working to express this as a new form of "associative iteration" that traverses the space of return types.
I mention these considerations because this approach to base changing resulted in very natural inter-operation with our SWAR framework, this is a strong indication that this approach is fundamentally "right", because the SWAR parallelism depends on associativity, and the minimum time complexity of collecting and integrating the partial results of computations made in parallel is in the order of the logarithm.

## Parallelism

The essence of parallel algorithms is the "mapping" of an operation that is associative to multiple data elements simultaneously or in parallel, and the collection (reduction, gathering, combination) of the partial results into the final result.

The combination of the partial results induce a tree structure, whose height is in the order of the logarithm of the number of elements N.
For a binary operation, the minimum height for this tree is the logarithm base 2 of the number of initial elements: for a binary operation, the minimum latency is achieved when at each pass we can reduce the data elements by half.

This is what happens in Lemire's method:  There are enough execution resources to halve 8 byte elements into 4.  The second stage reduces 4 partial results into 2.
It is clear that with the mathematical technique of squaring the base, **this mechanism is optimal with regards to latency** compared to anything else that uses this numeric base progression scheme.

A comparison with a fundamentally different technique is to revert to the linear, serial mechanism of "multiplying by 10 the previous result and adding the current digit".  That obviously leads to a latency of 7 partial results, multiplication is present in the stages of both schemes, and multiplication is the dominant latency[^1], 7 is worse than 3, so, Lemire's mechanism has, relatively, much lower, not higher, latency.

Then Lemire is comparing his mechanism to some unknown mechanism that might have smaller latency that can not be the normal mathematics of multiplication by a base.  The mathematical need is to convert numbers in base 10 into base 2.  There may be exotic mathematics for base conversions that use other methods, perhaps something based on fourier transform as the best multiplication algorithms, but for sure would be practical only for immense numbers of digits.

No, Lemire's is, for practical applications, the minimum latency.  At each stage of gathering results, the multiplication is the dominant latency.  Can we devise processing stages of smaller latencies?

I strongly suspect not, since the cheapest instructions such as bitwise operations and additions have latencies in the order of 1 cycle.  This method requires 1 multiplication, 1 mask, and 1 shift per stage, not more than 6 cycles, let's say that all of the register renaming, constant loading take one cycle.  That's 7 cycles per stage, and the number of stages is minimal.  See that multiplying by 10 as an addition chain requires 5 steps, so, that's a dead end.  Even the trick of using "Load Effective Address" in x86-64 using its very complex addressing modes could calculate the necessary things for base 10 conversion, but with instruction counts that won't outperform the 64 bit multiplication in each Lemire's stage.  We can see this in the [compiler explorer](https://godbolt.org/z/PjMoGzbPa), multiplying by 10 might be implemented with just 2 instructions of minimal latency (adding and `LEA`), but the compiler optimizer would not multiply by 100 without using the actual multiplication instruction.

See the appendix where we list the generated code in x86-64 for multiplications of factors up to 100, you'll see that the optimizer seems to "give up" on addition chains using the `LEA` instruction and the indexed addressing modes at a rate of four instructions of addition chains for one multiplication.  Since other architectures can at most give the complexity that x86-64 `LEA` and addressing modes, we can be **certain that there isn't a "budget" of non-multiplication operations that would outperform the base conversion that relies on multiplcation.

Lemire's point of view that his mechanism has high latency seems to be just wrong.

This example goes to show why SWAR mechanisms also have good latencies:  If the SWAR operation is the parallel mapping of computation and the parallel combination of partial results, they very well be of optimal latency.

## How [hardware SIMD](../../glossary.md#hardware-simd) leads to inefficiences due to design mistakes

Sometimes there is no need to combine partial results, for example, in `strlen`, we don't need to, for example, calculate the number of bytes that are null, we just want the first null; any good SIMD programming interface would give you "horizontal" operations to extract the desired result directly.

Curiously enough, our SWAR library gives that capability directly; in the `strlen` case, the operation is "count trailing zeros", however, most SIMD architectures, what we call "hardware SIMD", do not!

I think our colleagues in GLIBC that are so fond of assembler (we have proven this since our equivalents are more than 90% just C++ with very topical use of intrinsics, but theirs are through and through assembler code) would have identified the architectural primitive that would have spared them to have to use general purpose computation to identify the index into the block where the first null occurs.  Since they need to create a "mask" of booleans, it seems that those horizontal primitives are missing from AVX2 and Arm Neon.

In our Robin Hood implementation there are many examples of our generation of a SIMD of boolean results and the immediate use of those booleans as SIMD values for further computation. I believe this helps our SWAR, ["software SIMD"](https://github.com/thecppzoo/zoo/blob/em/essay-swar-latency.md/glossary.md#software-simd) implementations to be competitive with the software that uses "hardware SIMD".

This also shows a notorious deficiency in the efforts to standardize in C++ the interfaces for hardware SIMD: The results of lane-wise processing (vertical processing) that generate booleans would generate "mask" values of types that are bitfields of the results of comparisons, not SIMD type values.  So, if you were to consume these results in further SIMD processing, you'd need to convert and adjust the bitfields in general purpose registers into SIMD registers, this "back and forth SIMD-scalar" conversions would add further performance penalties that compiler optimizers might not be able to eliminate, for a variety of reasons that are out of the scope of this document, wait for us to write an essay that contains the phrase "cuckoo-bananas".

Our SWAR library, in this regard, is "future proof": boolean results are just an specific form of SIMD values.  We use this already for SIMD operations such as saturated addition: unsigned overflow (carry) is detected lane wise and lane-wise propagated to the whole lane, without conversion to scalar.  Saturation is implemented seamlessly as after-processing of computation, without having to convert to "scalar" (the result mask) and back.

In the early planning stages for support of "hardware" SIMD (architectures that do SIMD), I've thought that perhaps we will have to keep our emulated SIMD implementations on "hardware SIMD" as a flavor for users to choose from, since they may be the only way to avoid performance penalties that are higher than the cost of emulation, to prevent the error in the architecture, an error that no standard library could avoid except synthetizing the operations as we do.

## A hard challenge

As with much of what superficially looks bad in our SWAR library, upon closer examination, we determine fairly rigorously that these are the best choices, the essential costs.  In that regard, it meets the primary mission of serving as a way to express in software all that we know about performance so that users can benefit the most from our expertise.


-ed

## Footnotes

[^1] We're beginning to work on explaining the basics of processor instructions and operations costs.

## Appendix, multiplication codegen for x86-64

For this code, multiplying by a compile-time constant, we get this generated code [nice Compiler Explorer link](https://godbolt.org/z/z3b7e1Tnx):

```c++
template<unsigned F>
auto multiply(uint64_t factor) {
    return factor * F;
}
```

Notice the optimizer uses mostly addition chains based on `LEA` and the indexing addressing mode together with shifts to avoid multiplications, up to when the chain has four instructions, more would fallback to multiplication:

```asm
auto multiply<0u>(unsigned long):                   # @auto multiply<0u>(unsigned long)
        xor     eax, eax
        ret
auto multiply<1u>(unsigned long):                   # @auto multiply<1u>(unsigned long)
        mov     rax, rdi
        ret
auto multiply<2u>(unsigned long):                   # @auto multiply<2u>(unsigned long)
        lea     rax, [rdi + rdi]
        ret
auto multiply<3u>(unsigned long):                   # @auto multiply<3u>(unsigned long)
        lea     rax, [rdi + 2*rdi]
        ret
auto multiply<4u>(unsigned long):                   # @auto multiply<4u>(unsigned long)
        lea     rax, [4*rdi]
        ret
auto multiply<5u>(unsigned long):                   # @auto multiply<5u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        ret
auto multiply<6u>(unsigned long):                   # @auto multiply<6u>(unsigned long)
        add     rdi, rdi
        lea     rax, [rdi + 2*rdi]
        ret
auto multiply<7u>(unsigned long):                   # @auto multiply<7u>(unsigned long)
        lea     rax, [8*rdi]
        sub     rax, rdi
        ret
auto multiply<8u>(unsigned long):                   # @auto multiply<8u>(unsigned long)
        lea     rax, [8*rdi]
        ret
auto multiply<9u>(unsigned long):                   # @auto multiply<9u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        ret
auto multiply<10u>(unsigned long):                  # @auto multiply<10u>(unsigned long)
        add     rdi, rdi
        lea     rax, [rdi + 4*rdi]
        ret
auto multiply<11u>(unsigned long):                  # @auto multiply<11u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rdi + 2*rax]
        ret
auto multiply<12u>(unsigned long):                  # @auto multiply<12u>(unsigned long)
        shl     rdi, 2
        lea     rax, [rdi + 2*rdi]
        ret
auto multiply<13u>(unsigned long):                  # @auto multiply<13u>(unsigned long)
        lea     rax, [rdi + 2*rdi]
        lea     rax, [rdi + 4*rax]
        ret
auto multiply<14u>(unsigned long):                  # @auto multiply<14u>(unsigned long)
        mov     rax, rdi
        shl     rax, 4
        sub     rax, rdi
        sub     rax, rdi
        ret
auto multiply<15u>(unsigned long):                  # @auto multiply<15u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rax + 2*rax]
        ret
auto multiply<16u>(unsigned long):                  # @auto multiply<16u>(unsigned long)
        mov     rax, rdi
        shl     rax, 4
        ret
auto multiply<17u>(unsigned long):                  # @auto multiply<17u>(unsigned long)
        mov     rax, rdi
        shl     rax, 4
        add     rax, rdi
        ret
auto multiply<18u>(unsigned long):                  # @auto multiply<18u>(unsigned long)
        add     rdi, rdi
        lea     rax, [rdi + 8*rdi]
        ret
auto multiply<19u>(unsigned long):                  # @auto multiply<19u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rdi + 2*rax]
        ret
auto multiply<20u>(unsigned long):                  # @auto multiply<20u>(unsigned long)
        shl     rdi, 2
        lea     rax, [rdi + 4*rdi]
        ret
auto multiply<21u>(unsigned long):                  # @auto multiply<21u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rdi + 4*rax]
        ret
auto multiply<22u>(unsigned long):                  # @auto multiply<22u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rdi + 4*rax]
        add     rax, rdi
        ret
auto multiply<23u>(unsigned long):                  # @auto multiply<23u>(unsigned long)
        lea     rax, [rdi + 2*rdi]
        shl     rax, 3
        sub     rax, rdi
        ret
auto multiply<24u>(unsigned long):                  # @auto multiply<24u>(unsigned long)
        shl     rdi, 3
        lea     rax, [rdi + 2*rdi]
        ret
auto multiply<25u>(unsigned long):                  # @auto multiply<25u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rax + 4*rax]
        ret
auto multiply<26u>(unsigned long):                  # @auto multiply<26u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rax + 4*rax]
        add     rax, rdi
        ret
auto multiply<27u>(unsigned long):                  # @auto multiply<27u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rax + 2*rax]
        ret
auto multiply<28u>(unsigned long):                  # @auto multiply<28u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rax + 2*rax]
        add     rax, rdi
        ret
auto multiply<29u>(unsigned long):                  # @auto multiply<29u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rax + 2*rax]
        add     rax, rdi
        add     rax, rdi
        ret
auto multiply<30u>(unsigned long):                  # @auto multiply<30u>(unsigned long)
        mov     rax, rdi
        shl     rax, 5
        sub     rax, rdi
        sub     rax, rdi
        ret
auto multiply<31u>(unsigned long):                  # @auto multiply<31u>(unsigned long)
        mov     rax, rdi
        shl     rax, 5
        sub     rax, rdi
        ret
auto multiply<32u>(unsigned long):                  # @auto multiply<32u>(unsigned long)
        mov     rax, rdi
        shl     rax, 5
        ret
auto multiply<33u>(unsigned long):                  # @auto multiply<33u>(unsigned long)
        mov     rax, rdi
        shl     rax, 5
        add     rax, rdi
        ret
auto multiply<34u>(unsigned long):                  # @auto multiply<34u>(unsigned long)
        mov     rax, rdi
        shl     rax, 5
        lea     rax, [rax + 2*rdi]
        ret
auto multiply<35u>(unsigned long):                  # @auto multiply<35u>(unsigned long)
        imul    rax, rdi, 35
        ret
auto multiply<36u>(unsigned long):                  # @auto multiply<36u>(unsigned long)
        shl     rdi, 2
        lea     rax, [rdi + 8*rdi]
        ret
auto multiply<37u>(unsigned long):                  # @auto multiply<37u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rdi + 4*rax]
        ret
auto multiply<38u>(unsigned long):                  # @auto multiply<38u>(unsigned long)
        imul    rax, rdi, 38
        ret
auto multiply<39u>(unsigned long):                  # @auto multiply<39u>(unsigned long)
        imul    rax, rdi, 39
        ret
auto multiply<40u>(unsigned long):                  # @auto multiply<40u>(unsigned long)
        shl     rdi, 3
        lea     rax, [rdi + 4*rdi]
        ret
auto multiply<41u>(unsigned long):                  # @auto multiply<41u>(unsigned long)
        lea     rax, [rdi + 4*rdi]
        lea     rax, [rdi + 8*rax]
        ret
auto multiply<42u>(unsigned long):                  # @auto multiply<42u>(unsigned long)
        imul    rax, rdi, 42
        ret
auto multiply<43u>(unsigned long):                  # @auto multiply<43u>(unsigned long)
        imul    rax, rdi, 43
        ret
auto multiply<44u>(unsigned long):                  # @auto multiply<44u>(unsigned long)
        imul    rax, rdi, 44
        ret
auto multiply<45u>(unsigned long):                  # @auto multiply<45u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rax + 4*rax]
        ret
auto multiply<46u>(unsigned long):                  # @auto multiply<46u>(unsigned long)
        imul    rax, rdi, 46
        ret
auto multiply<47u>(unsigned long):                  # @auto multiply<47u>(unsigned long)
        imul    rax, rdi, 47
        ret
auto multiply<48u>(unsigned long):                  # @auto multiply<48u>(unsigned long)
        shl     rdi, 4
        lea     rax, [rdi + 2*rdi]
        ret
auto multiply<49u>(unsigned long):                  # @auto multiply<49u>(unsigned long)
        imul    rax, rdi, 49
        ret
auto multiply<50u>(unsigned long):                  # @auto multiply<50u>(unsigned long)
        imul    rax, rdi, 50
        ret
auto multiply<51u>(unsigned long):                  # @auto multiply<51u>(unsigned long)
        imul    rax, rdi, 51
        ret
auto multiply<52u>(unsigned long):                  # @auto multiply<52u>(unsigned long)
        imul    rax, rdi, 52
        ret
auto multiply<53u>(unsigned long):                  # @auto multiply<53u>(unsigned long)
        imul    rax, rdi, 53
        ret
auto multiply<54u>(unsigned long):                  # @auto multiply<54u>(unsigned long)
        imul    rax, rdi, 54
        ret
auto multiply<55u>(unsigned long):                  # @auto multiply<55u>(unsigned long)
        imul    rax, rdi, 55
        ret
auto multiply<56u>(unsigned long):                  # @auto multiply<56u>(unsigned long)
        imul    rax, rdi, 56
        ret
auto multiply<57u>(unsigned long):                  # @auto multiply<57u>(unsigned long)
        imul    rax, rdi, 57
        ret
auto multiply<58u>(unsigned long):                  # @auto multiply<58u>(unsigned long)
        imul    rax, rdi, 58
        ret
auto multiply<59u>(unsigned long):                  # @auto multiply<59u>(unsigned long)
        imul    rax, rdi, 59
        ret
auto multiply<60u>(unsigned long):                  # @auto multiply<60u>(unsigned long)
        imul    rax, rdi, 60
        ret
auto multiply<61u>(unsigned long):                  # @auto multiply<61u>(unsigned long)
        imul    rax, rdi, 61
        ret
auto multiply<62u>(unsigned long):                  # @auto multiply<62u>(unsigned long)
        mov     rax, rdi
        shl     rax, 6
        sub     rax, rdi
        sub     rax, rdi
        ret
auto multiply<63u>(unsigned long):                  # @auto multiply<63u>(unsigned long)
        mov     rax, rdi
        shl     rax, 6
        sub     rax, rdi
        ret
auto multiply<64u>(unsigned long):                  # @auto multiply<64u>(unsigned long)
        mov     rax, rdi
        shl     rax, 6
        ret
auto multiply<65u>(unsigned long):                  # @auto multiply<65u>(unsigned long)
        mov     rax, rdi
        shl     rax, 6
        add     rax, rdi
        ret
auto multiply<66u>(unsigned long):                  # @auto multiply<66u>(unsigned long)
        mov     rax, rdi
        shl     rax, 6
        lea     rax, [rax + 2*rdi]
        ret
auto multiply<67u>(unsigned long):                  # @auto multiply<67u>(unsigned long)
        imul    rax, rdi, 67
        ret
auto multiply<68u>(unsigned long):                  # @auto multiply<68u>(unsigned long)
        mov     rax, rdi
        shl     rax, 6
        lea     rax, [rax + 4*rdi]
        ret
auto multiply<69u>(unsigned long):                  # @auto multiply<69u>(unsigned long)
        imul    rax, rdi, 69
        ret
auto multiply<70u>(unsigned long):                  # @auto multiply<70u>(unsigned long)
        imul    rax, rdi, 70
        ret
auto multiply<71u>(unsigned long):                  # @auto multiply<71u>(unsigned long)
        imul    rax, rdi, 71
        ret
auto multiply<72u>(unsigned long):                  # @auto multiply<72u>(unsigned long)
        shl     rdi, 3
        lea     rax, [rdi + 8*rdi]
        ret
auto multiply<73u>(unsigned long):                  # @auto multiply<73u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rdi + 8*rax]
        ret
auto multiply<74u>(unsigned long):                  # @auto multiply<74u>(unsigned long)
        imul    rax, rdi, 74
        ret
auto multiply<75u>(unsigned long):                  # @auto multiply<75u>(unsigned long)
        imul    rax, rdi, 75
        ret
auto multiply<76u>(unsigned long):                  # @auto multiply<76u>(unsigned long)
        imul    rax, rdi, 76
        ret
auto multiply<77u>(unsigned long):                  # @auto multiply<77u>(unsigned long)
        imul    rax, rdi, 77
        ret
auto multiply<78u>(unsigned long):                  # @auto multiply<78u>(unsigned long)
        imul    rax, rdi, 78
        ret
auto multiply<79u>(unsigned long):                  # @auto multiply<79u>(unsigned long)
        imul    rax, rdi, 79
        ret
auto multiply<80u>(unsigned long):                  # @auto multiply<80u>(unsigned long)
        shl     rdi, 4
        lea     rax, [rdi + 4*rdi]
        ret
auto multiply<81u>(unsigned long):                  # @auto multiply<81u>(unsigned long)
        lea     rax, [rdi + 8*rdi]
        lea     rax, [rax + 8*rax]
        ret
auto multiply<82u>(unsigned long):                  # @auto multiply<82u>(unsigned long)
        imul    rax, rdi, 82
        ret
auto multiply<83u>(unsigned long):                  # @auto multiply<83u>(unsigned long)
        imul    rax, rdi, 83
        ret
auto multiply<84u>(unsigned long):                  # @auto multiply<84u>(unsigned long)
        imul    rax, rdi, 84
        ret
auto multiply<85u>(unsigned long):                  # @auto multiply<85u>(unsigned long)
        imul    rax, rdi, 85
        ret
auto multiply<86u>(unsigned long):                  # @auto multiply<86u>(unsigned long)
        imul    rax, rdi, 86
        ret
auto multiply<87u>(unsigned long):                  # @auto multiply<87u>(unsigned long)
        imul    rax, rdi, 87
        ret
auto multiply<88u>(unsigned long):                  # @auto multiply<88u>(unsigned long)
        imul    rax, rdi, 88
        ret
auto multiply<89u>(unsigned long):                  # @auto multiply<89u>(unsigned long)
        imul    rax, rdi, 89
        ret
auto multiply<90u>(unsigned long):                  # @auto multiply<90u>(unsigned long)
        imul    rax, rdi, 90
        ret
auto multiply<91u>(unsigned long):                  # @auto multiply<91u>(unsigned long)
        imul    rax, rdi, 91
        ret
auto multiply<92u>(unsigned long):                  # @auto multiply<92u>(unsigned long)
        imul    rax, rdi, 92
        ret
auto multiply<93u>(unsigned long):                  # @auto multiply<93u>(unsigned long)
        imul    rax, rdi, 93
        ret
auto multiply<94u>(unsigned long):                  # @auto multiply<94u>(unsigned long)
        imul    rax, rdi, 94
        ret
auto multiply<95u>(unsigned long):                  # @auto multiply<95u>(unsigned long)
        imul    rax, rdi, 95
        ret
auto multiply<96u>(unsigned long):                  # @auto multiply<96u>(unsigned long)
        shl     rdi, 5
        lea     rax, [rdi + 2*rdi]
        ret
auto multiply<97u>(unsigned long):                  # @auto multiply<97u>(unsigned long)
        imul    rax, rdi, 97
        ret
auto multiply<98u>(unsigned long):                  # @auto multiply<98u>(unsigned long)
        imul    rax, rdi, 98
        ret
auto multiply<99u>(unsigned long):                  # @auto multiply<99u>(unsigned long)
        imul    rax, rdi, 99
        ret
auto multiply<100u>(unsigned long):                 # @auto multiply<100u>(unsigned long)
        imul    rax, rdi, 100
        ret
```
