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

See this diagram, not explaining Lemire's technique itself but the process of mapping parallel computation and collecting partial results halving their quantity each step:

```
[ a | b | c | d | e | f | g | h ] <-- input bytes containing decimal digits (the most significant digit is a
    |       |       |       |     <- multiplication "mapped" in parallel, generating half as many partial results
    v       v       v       v
[a*10+b | c*10+d| e*10+f| g*10+h] ==
[  P1   |   P2  |   P3  |   P4  ]      <- partial results renamed
[ P1*10^2 + P2  |  P3*10^2 + P4 ]      <- halving of the number of partial results
[(P1*10^2 + P2)*10^4 + (P3*10^2 + P4)] <- final halving: one result
Summary: the operation mapped in parallel, each combination or reduction step halves the number of partial results
Repeated halving: log2(N) steps.
```

This is what happens in Lemire's method:  There are enough execution resources to halve 8 byte elements into 4, or fully supporting a width of 8 [^1].  The second stage reduces 4 partial results into 2, the final step combines two partial results into one.

It is clear that with the mathematical technique of squaring the base, **this mechanism is optimal with regards to latency** compared to anything else that uses this numeric base progression scheme.

A comparison with a fundamentally different technique is to revert to the linear, serial mechanism of "multiplying by 10 the previous result and adding the current digit".  That obviously leads to a latency of 7 partial results, multiplication is present in the stages of both schemes, and multiplication is the dominant latency[^2], 7 is worse than 3, so, Lemire's mechanism has, relatively, much lower, not higher, latency.

Then Lemire's comment must mean he might be speculating that there might be some unknown mechanism that might have smaller latency that can not be the normal mathematics of multiplication by a base.  The mathematical need is to convert numbers in base 10 into base 2.  There may be exotic mathematics for base conversions that use other methods, perhaps something based on fourier transform as the best multiplication algorithms, but for sure would be practical only for immense numbers of digits.

No, Lemire's has, for practical applications, the minimum number of stages.  If we can not improve the stages themselves, then the technique as it is could be optimal.  What dominates the latency at each stage?: multiplication.  Can we devise processing stages of smaller latencies?

I strongly suspect not, since the cheapest instructions such as bitwise operations and additions have latencies in the order of 1 cycle.  This method requires 1 multiplication, 1 mask, and 1 shift per stage, not more than 6 cycles, let's say that all of the register renaming, constant loading take one cycle.  That's 7 cycles per stage, and the number of stages is minimal.  See that multiplying by 10 as an addition chain requires 5 steps, so, that's a dead end.  Even the trick of using "Load Effective Address" in x86-64 using its very complex addressing modes could calculate the necessary things for base 10 conversion, but with instruction counts that won't outperform the 64 bit multiplication in each Lemire's stage.  We can see this in the [compiler explorer](https://godbolt.org/z/PjMoGzbPa), multiplying by 10 might be implemented with just 2 instructions of minimal latency (shifting, adding and `LEA`), but the compiler optimizer would not multiply by 100 without using the actual multiplication instruction.

See the appendix where we list the generated code in x86-64 for multiplications of factors up to 100, you'll see that the optimizer seems to "give up" on addition chains using the `LEA` instruction and the indexed addressing modes at a rate of four instructions of addition chains for one multiplication.  Since other architectures can at most give the complexity that x86-64 `LEA` and addressing modes has, we can be **certain that there isn't a "budget" of non-multiplication operations that would outperform** the base conversion that relies on multiplcation.

Lemire's point of view that his mechanism has high latency seems to be just wrong.

This example goes to show why SWAR mechanisms also have good latencies:  If the SWAR operation is the parallel mapping of computation and the parallel combination of partial results, they very well be of optimal latency.

## How [hardware SIMD](../../glossary.md#hardware-simd) leads to inefficiences due to design mistakes

Sometimes there is no need to combine partial results, for example, in `strlen`, we don't need to calculate the number of bytes that are null, we just want the first null; any good SIMD programming interface would give you "horizontal" operations to extract the desired result directly.

Curiously enough, our SWAR library gives that capability directly; in the `strlen` case, the operation is "count trailing zeros", however, most SIMD architectures, what we call "hardware SIMD", do not!

I think our colleagues in GLIBC that are so fond of assembler (we have proven this since our equivalents are more than 90% just C++ with very topical use of intrinsics, but theirs are through and through assembler code) would have identified the architectural primitive that would have spared them to have to use general purpose computation to identify the index into the block where the first null occurs.  Since they need to create a "mask" of booleans, it seems that those horizontal primitives are missing from AVX2 and Arm Neon.

In our Robin Hood implementation there are many examples of our generation of a SIMD of boolean results and the immediate use of those booleans as SIMD values for further computation. I believe this helps our SWAR, ["software SIMD"](https://github.com/thecppzoo/zoo/blob/em/essay-swar-latency.md/glossary.md#software-simd) implementations to be competitive with the software that uses "hardware SIMD".

This also shows a notorious deficiency in the efforts to standardize in C++ the interfaces for hardware SIMD: The results of lane-wise processing (vertical processing) that generate booleans would generate "mask" values of types that are bitfields of the results of comparisons, an "scalar" to be used in [ the non-vector "General Purpose Registers", GPRs, not SIMD type values [^3].  So, if you were to consume these results in further SIMD processing, you'd need to convert and adjust the bitfields in general purpose registers into SIMD registers, this "back and forth SIMD-scalar" conversions would add further performance penalties that compiler optimizers might not be able to eliminate, for a variety of reasons that are out of the scope of this document. Wait for us to write an essay that contains the phrase "cuckoo-bananas".

Our SWAR library, in this regard, is "future proof" [^4]: boolean results are just an specific form of SIMD values.  We use this already for SIMD operations such as saturated addition: unsigned overflow (carry) is detected lane wise and lane-wise propagated to the whole lane, without conversion to scalar.  Saturation is implemented seamlessly as after-processing of computation, without having to convert to "scalar" (the result mask) and back.

In the early planning stages for support of "hardware" SIMD (architectures that do SIMD), I've thought that perhaps we will have to keep our emulated SIMD implementations on "hardware SIMD" as a flavor for users to choose from, since they may be the only way to avoid performance penalties that are higher than the cost of emulation, to prevent the error in the architecture, an error that no standard library could avoid except synthetizing the operations as we do.

## A hard challenge

As with much of what superficially may look bad in our SWAR library, upon closer examination, we determine fairly rigorously that these are the best choices, the essential costs.  In that regard, it meets the primary mission of serving as a way to express in software all that we know about performance so that users can benefit the most from our expertise


-ed

## Footnotes

### Execution Resources

[^1]: Go to ["Execution Resources"](execution-resources)

We say "there are enough execution resources to consume 8 bytes of inputs at a time", we mean that multiplication of 64 bits seems to be as performant as 32 bits in most architectures we have used.  One possible exception might be GPUs: they might double the latency for 64 bit multiplications compared to 32.  We implemented Lemire's technique for 128 bits relatively unsuccessfully: 64 bit processors generally have instructions to help with 128 bits multiplies, for example, x86_64 gives you the result in the pair of registers `rDX:rAX` each of 64 bits.  We use this in the best 128 bit calque of the technique.  The straightforward use of full 128 bit multiplication requires three multiplications: two 64-to-64 bit, and one 64 bit factors to 128 bit result, this is better than the otherwise 2<sup>2</sup> (the usual multiplication algorihtm requires N<sup>2</sup> products), but most importantly, greater than 2, so, doubling the width performs worse because the critical operation takes 3 times as much, that's what we mean by having or not having execution resources for a given width:
```c++
auto multiply64_to128_bits(uint64_t f1, uint64_t f2) {
    return __uint128_t(f1) * f2;
}

auto multiply128_to128_bits(__uint128_t f1, __uint128_t f2) {
    return f1 * f2;
}
```
translates to, via the [Compiler Explorer](https://godbolt.org/z/ofP71xzj9)
```asm
multiply64_to128_bits(unsigned long, unsigned long):            # @multiply64_to128_bits(unsigned long, unsigned long)
        mov     rdx, rsi
        mulx    rdx, rax, rdi
        ret
multiply128_to128_bits(unsigned __int128, unsigned __int128):           # @multiply128_to128_bits(unsigned __int128, unsigned __int128)
        imul    rsi, rdx
        mulx    rdx, rax, rdi
        imul    rcx, rdi
        add     rcx, rsi
        add     rdx, rcx
        ret
```

[^2]: We're beginning to work on explaining the basics of processor instructions and operations costs.

### Round trips between SIMD and GPR

[^3]: See: [SIMD-GPR round trips](round-trips-between-simd-and-gpr)

In AVX/AVX2 (of x86-64) typically a SIMD comparison such as "greater than" would generate a mask: for inputs `a` and `b`, the assembler would look like this (ChatGPT generated):
```asm
_start:
    ; Load vectors a and b into ymm registers
    vmovdqa ymm0, [a]
    vmovdqa ymm1, [b]
    
    ; Step 1: Generate the mask using a comparison
    vpcmpgtd ymm2, ymm0, ymm1
    
    ; Step 2: Extract the mask bits into an integer
    vpmovmskb eax, ymm2
    
    ; Step 3: Broadcast the mask bits into a SIMD register
    vmovd xmm3, eax
    vpbroadcastd ymm3, xmm3
```
See the inefficiency?
For ARM Neon and the RISC-V ISAs, it is more complicated, because their ISAs do not have an instruction for broadcasting one bit from a mask to every lane.
I'm confident that this design mistake might not be solved any time soon, here's why:
Take AVX512, for example, the latest and best of x86_64 SIMD:  in AVX512, there are mask registers, of 64 bits, so they can map one bit to every byte-lane of a `ZMM`register.  It recognizes the need to prevent computation on certain lanes, the ones that are "masked out" by the mask register.  However, this is not the same thing as having a bit in the lane itself that you can use in computation.  To do that, you need to go to the GPRs moving a mask, back to the `ZMM` registers via a move followed by a bit-broadcast, just like in AVX/AVX2.  The reason why apparently nobody else has "felt" this pain might be, I speculate, because SIMD is mostly used with floating point numbers.  It wouldn't make sense to turn on a bit in lanes.
Our SWAR work demonstrates that there are plentiful opportunities to turn normally serial computation into ad-hoc parallelism, for this we use integers always, and we can really benefit from the lanes themselves having a bit in them, as opposed to predicating each lane by a mask.  Our implementation of saturated addition is one example: we detect overflow (carry) as a `BooleanSWAR`, we know that is the most significant bit of each lane, we can copy down that bit by subtracting the MSB as the LSB of each lane.

### Future Proofing

[^4]: See ["Future Proof"](future-proofing)

When we mean "future proof" we mean that the discernible tendencies agree with our approach:  Fundamental physics is impeding further miniaturization of transistors, I speculate that, for example, the excution units dedicated to SIMD and "scalar" or General Purpose operations would have to be physically separated in the chip, moving data from one side to the other would be ever more expensive.  Actually, In x86-64 similar penalties existed such as having the result of a floating point operation being an input to an integer operation, changing the "type" of value held, back when AVX and AVX2 were relatively new, exhibited a penalty that was not negligible.  Because the execution units for FP and Integers were different.  This changed, nowadays they use the same execution units, as evidenced in the port allocations for the microarchitectures, but I conjecture that "something has to give", I see that operations on 64 bit "scalar" integers and 512 bit `ZMM` values with internal structure of lanes are so different that "one type fits all" execution units would be less effective.  We can foresee ever wider SIMD computation, ARM's design has an extension that would allow immense widths of 2048 bits of SIMD, it is called "Scalable Vector Extensions", RISC-V has widening baked into the very design, they call it "Vector Length Agnostic" with its acronym VLA, so that you program in a way that is not specific to a width... Just thinking about the extreme challenge of merely propagating signals in buses of 2048, that the wider they are they also have to be longer, that the speed of signal propagation, the speed of light, the maxium speed in the universe, has been a limiting factor that is ever more limiting; I really think that the round trip of SIMD boolean lane-wise test to mask in SIMD register to GPR and back to SIMD will be more significant, our SWAR library already avoids this.  Yet, it is crucial to work with the result of boolean tests in a SIMD way, this is the key to branchless programming techniques, the only way to avoid control dependencies.

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
