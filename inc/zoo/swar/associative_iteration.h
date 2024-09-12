#ifndef ZOO_SWAR_ASSOCIATIVE_ITERATION_H
#define ZOO_SWAR_ASSOCIATIVE_ITERATION_H

#include "zoo/swar/SWAR.h"

//#define ZOO_DEVELOPMENT_DEBUGGING
#ifdef ZOO_DEVELOPMENT_DEBUGGING
#include <iostream>

inline std::ostream &binary(std::ostream &out, uint64_t input, int count) {
    while(count--) {
        out << (1 & input);
        input >>= 1;
    }
    return out;
}

template<int NB, typename B>
std::ostream &operator<<(std::ostream &out, zoo::swar::SWAR<NB, B> s) {
    using S = zoo::swar::SWAR<NB, B>;
    auto shiftCounter = sizeof(B) * 8 / NB;
    out << "<|";
    auto v = s.value();
    do {
        binary(out, v, NB) << '|';

    } while(--shiftCounter);
    return out << ">";
}

#define ZOO_TO_STRING(a) #a
// std::endl is needed within the context of debugging: flush the line
#define ZOO_TRACEABLE_EXP_IMPL(F, L, ...) std::cout << '"' << (__VA_ARGS__) << "\", \"" <<  F << ':' << L << "\", \"" << ZOO_TO_STRING(__VA_ARGS__) << "\"" << std::endl;
#define ZOO_TRACEABLE_EXPRESSION(...) ZOO_TRACEABLE_EXP_IMPL(__FILE__, __LINE__, __VA_ARGS__)

#else

#define ZOO_TRACEABLE_EXPRESSION(...) (void)(__VA_ARGS__)

#endif

namespace zoo::swar {

constexpr auto log2_of_power_of_two = [](auto power_of_two) {
    if (power_of_two == 0) {
        return 0;
    }
    if (power_of_two == 1) {
        return 1;
    }
    return __builtin_ctz(power_of_two);
};

enum class ParallelXixOperation {
    Suffix,
    Prefix
};

template<ParallelXixOperation XixType, typename S>
constexpr auto parallelXix(S input) {
    constexpr auto operation = [] (auto doubling, auto power, auto mask) {
        auto shifted = [&] {
            if constexpr(XixType == ParallelXixOperation::Suffix) {
                return doubling.shiftIntraLaneLeft(power, mask);
            }
            return doubling.shiftIntraLaneRight(power, mask);
        }();
        doubling = doubling ^ shifted;
        return doubling;
    };
    auto log2Count = log2_of_power_of_two(S::NBits),
         power = 1;
    auto result = input,
         mask = S{~S::MostSignificantBit};
    for(;;) {
        result = operation(result, power, mask);
        if (!--log2Count) {
            break;
        }
        mask = mask & S{mask.value() >> power};
        power <<= 1;
    }
    return S{result};
}

// not 100% sure this one works yet, need to test
// not sure that the power needs to shift the mask which way, and if the
// mask needs to start with the least significant bit etc.
template<typename S>
constexpr auto parallelPrefix(S input) {
    return parallelXix<ParallelXixOperation::Prefix>(input);
}

template<typename S>
constexpr auto parallelSuffix(S input) {
    return parallelXix<ParallelXixOperation::Suffix>(input);
}

static_assert(
    parallelSuffix(SWAR<32, u64>{
        0b00000000000000110000000000000011'00000000000000000000000000000111}).value()
     == 0b00000000000000010000000000000001'11111111111111111111111111111101
);

static_assert(
    parallelSuffix(SWAR<16, u32>{
        0b0000000000000011'0000000000000011}).value()
     == 0b0000000000000001'0000000000000001
);

static_assert(
    parallelSuffix(SWAR<8, u32>{
        0b00000000'00000000'00000000'00000000}).value()
     == 0b00000000'00000000'00000000'00000000
);

static_assert(
    parallelSuffix(SWAR<8, u32>{
        0b00000011'00000011'00000111'00000011}).value()
     == 0b00000001'00000001'11111101'00000001
);

static_assert(
    parallelSuffix(SWAR<8, u32>{
        0b00011000'00000011'00111000'00000011}).value()
     == 0b00001000'00000001'11101000'00000001
);

static_assert(
    parallelSuffix(SWAR<4, u32> {
        0b0011'0110'0011'0000'0110'0011'0011'0011}).value()
     == 0b0001'0010'0001'0000'0010'0001'0001'0001
);



/*
Binary compress: A fascinating algorithm.

Warren (Hacker's Delight) believes Guy L. Steele is the author of the following
binary compression operation, equivalent to Intel's BMI2 instruction PEXT of
"Parallel Extraction"

From a "mask", a selector of bits from an input, we want to put them together in
the output.

For example's sake, this is the selector:
Note: this follows the usual 'big endian' convention of denoting the most
significant bit first:
0001 0011 0111 0111 0110 1110 1100 1010
Imagine the input is the 32-bit or 32-boolean variable expression
abcd efgh ijkl mnop qrst uvxy zABC DEFG
We want the selection
   d   gh  jkl  nop  rs  uvx  zA   D F
To be compressed into the output
0000 0000 0000 00dg hjkl nopr suvx zADF

This algorithm will virtually calculate the count of positions that the selected
bits travel to the right, by constructing the binary encoding of that count:
It will identify the positions that will travel an odd number of positions to
the right, these are those whose position-travel-count have the units set.
It will then move those positions by one position to the right, and eliminate
them from the yet-to-move positions.  Because it eliminates the positions that
would move an odd count, there remains only positions that move an even number
of positions.  Now it finds the positions that move an odd count of /pairs/ of
positions, it moves them 2 positions.  This is equivalent to finding the
positions that would have the bit for 2 set in the count of positions to move
right.
Then an odd count of /quartets/ of positions, and moves them 4;
8, 16, 32, ...

Complete example (32 bits)
Selection mask:
0001 0011 0111 0111 0110 1110 1100 1010
Input (each letter or variable is a boolean, that can have 0 or 1)
abcd efgh ijkl mnop qrst uvxy zABC DEFG
Selection (using spaces)
   d   gh  jkl  nop  rs  uvx  zA   D F
Desired result:
                     dghjklnoprsuvxzADF

0001 0011 0111 0111 0110 1110 1100 1010 compressionMask
1110 1100 1000 1000 1001 0001 0011 0101 ~compressionMask
1101 1001 0001 0001 0010 0010 0110 1010 forParallelSuffix == mk == shiftleft 1
                                            == groupsize of ~compressionMask
This indicates the positions that have a 0 immediately to the right in
                                            compressionMask
4322 1000 9999 8888 7765 5554 4432 2110 number of 1s at and to the right of the
                                          current position in forParallelSuffix,
                                          last decimal digit
0100 1000 1111 0000 1101 1110 0010 0110 mp == parallel suffix of
                                            forParallelSuffix
We have just identified the positions that need to move an odd number of
positions.  Filter them with positions with a bit set in compressionMask:
0001 0011 0111 0111 0110 1110 1100 1010 compressionMask
---- ---- -111 ---- -1-- 111- ---- --1- mv == move (compress) these bits of
                                            compressionMask by 1 == groupSize
0001 0011 0000 0111 0010 0000 1100 1000 mv ^ compressionMask (clear the bits
                                            that will move)
---- ---- --11 1--- --1- -111 ---- ---1 mv >> 1 == groupSize
0001 0011 0011 1111 0010 0111 1100 1001 pseudo-compressed compressionMask.
0100 1000 1111 0000 1101 1110 0010 0110 mp == parallel suffix of
                                            forParallelSuffix
1011 0111 0000 1111 0010 0001 1101 1001 ~mp == ~parallel suffix (bits not moved)
1101 1001 0001 0001 0010 0010 0110 1010 forParallelSuffix (remember: had a zero
                                            immediately to their right)
1001 0001 0000 0001 0010 0000 0100 1000 new forParallelSuffix (also not moved =>
                                                had even zeroes to their right)
At this point, we have removed from compressionMask the positions that moved an
odd number of positions and moved them 1 position,
then, we only keep positions that move an even number of positions.
Now, we will repeat these steps but for groups of two zeroes, then 4 zeroes, ...
*/

template<int NB, typename B>
constexpr SWAR<NB, B>
compress(SWAR<NB, B> input, SWAR<NB, B> compressionMask) {
    // This solution uses the parallel suffix operation as a primary tool:
    // For every bit postion it indicates an odd number of ones to the right,
    // including itself.
    // Because we want to detect the "oddness" of groups of zeroes to the right,
    // we flip the compression mask.  To not count the bit position itself,
    // we shift by one.
    #define ZTE(...)
        // ZOO_TRACEABLE_EXPRESSION(__VA_ARGS__)
    ZTE(input);
    ZTE(compressionMask);
    using S = SWAR<NB, B>;
    auto result = input & compressionMask;
    auto groupSize = 1;
    auto
        shiftLeftMask = S{S::LowerBits},
        shiftRightMask = S{S::LowerBits << 1};
    ZTE(~compressionMask);
    auto forParallelSuffix = // this is called "mk" in the book
        (~compressionMask).shiftIntraLaneLeft(groupSize, shiftLeftMask);
    ZTE(forParallelSuffix);
        // note: forParallelSuffix denotes positions with a zero
        // immediately to the right in 'compressionMask'
    for(;;) {
        ZTE(groupSize);
        ZTE(shiftLeftMask);
        ZTE(shiftRightMask);
        ZTE(result);
        auto oddCountOfGroupsOfZerosToTheRight =  // called "mp" in the book
            parallelSuffix(forParallelSuffix);
        ZTE(oddCountOfGroupsOfZerosToTheRight);
        // compress the bits just identified in both the result and the mask
        auto moving = compressionMask & oddCountOfGroupsOfZerosToTheRight;
        ZTE(moving);
        compressionMask =
            (compressionMask ^ moving) | // clear the moving
            moving.shiftIntraLaneRight(groupSize, shiftRightMask);
        ZTE(compressionMask);
        auto movingFromInput = result & moving;
        result =
            (result ^ movingFromInput) | // clear the moving from the result
            movingFromInput.shiftIntraLaneRight(groupSize, shiftRightMask);
        auto nextGroupSize = groupSize << 1;
        if(NB <= nextGroupSize) {
            break;
        }
        auto evenCountOfGroupsOfZerosToTheRight =
            ~oddCountOfGroupsOfZerosToTheRight;
        forParallelSuffix =
            forParallelSuffix & evenCountOfGroupsOfZerosToTheRight;
        auto newShiftLeftMask =
            shiftLeftMask.shiftIntraLaneRight(groupSize, shiftRightMask);
        shiftRightMask =
            shiftRightMask.shiftIntraLaneLeft(groupSize, shiftLeftMask);
        shiftLeftMask = newShiftLeftMask;
        groupSize = nextGroupSize;
    }
    ZTE(result);
    #undef ZTE
    return result;
}

/// \todo because of the desirability of "accumuating" the XORs at the MSB,
/// the parallel suffix operation is more suitable.
template<int NB, typename B>
constexpr SWAR<NB, B> parity(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto preResult = parallelSuffix(input);
    auto onlyMSB = preResult.value() & S::MostSignificantBit;
    return S{onlyMSB};
}


namespace impl {
template<int NB, typename B>
constexpr auto makeLaneMaskFromMSB_and_LSB(SWAR<NB, B> msb, SWAR<NB, B> lsb) {
    auto msbCopiedDown = msb - lsb;
    auto msbReintroduced = msbCopiedDown | msb;
    return msbReintroduced;
}
}

template<int NB, typename B>
constexpr auto makeLaneMaskFromLSB(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto lsb = input & S{S::LeastSignificantBit};
    auto lsbCopiedToMSB = S{lsb.value() << (NB - 1)};
    return impl::makeLaneMaskFromMSB_and_LSB(lsbCopiedToMSB, lsb);
}

template<int NB, typename B>
constexpr auto makeLaneMaskFromMSB(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto msb = input & S{S::MostSignificantBit};
    auto msbCopiedToLSB = S{msb.value() >> (NB - 1)};
    return impl::makeLaneMaskFromMSB_and_LSB(msb, msbCopiedToLSB);
}

template<int NB, typename B>
struct ArithmeticResultTriplet {
    SWAR<NB, B> result;
    BooleanSWAR<NB, B> carry, overflow;
};


/// \brief "Safe" addition, meaning non-corrupting unsigned overflow addition
/// and producing the flags for unsigned overflow (carry) and signed overflow.
///
/// This is the function to perform signed addition (that relies on supporting
/// unsigned overflow)
///
/// Notice that the support for unsigned overflow directly allows signed
/// addition (and both signed and unsigned substraction).
///
/// This function is called "full addition" because it can perform the addition
/// with all the bits of the inputs by making sure the overflow (in the
/// unsigned sense) does not cross the lane boundary.
/// This function has less performance than "optimistic" addition (operator+).
/// The mechanism to manage potential overflow naturally allows the calculation
/// of the carry and signed overflow flags for no extra performance cost.
///
/// The performance relies on the optimizer removing the calculation of
/// the carry or signed overflow if they are not used.
///
/// When interpreted as unsigned addition, carrying out of the result is
/// overflow.
///
/// The carry bit is essential to increase the precision of the results in
/// normal arithmetic, but in unsigned SWAR it is preferable to double the
/// precision before executing addition, thus guaranteeing no overflow will
/// occur and using the more performant operator+ addition.  Hence,
/// the carry flag is mostly useful in SWAR for detection of unsigned overflow,
/// as opposed to a helper to achieve higher precision as it is normally used:
/// Once we support non-power-of-two sizes of lanes in number of bits, we could
/// have transformations that would convert a SWAR of lanes of N bits to a SWAR
/// of (N + 1) bits, however, functionally, the performance cost of doing this
/// will be strictly higher than doubling the bit count of the current SWAR.
/// With double the number of bits, not only are additions guaranteed to not
/// overflow (as unsigned) but even multiplications won't.  Then it is not
/// practical to use the carry bit for other than detection of unsigned overflow.
///
/// The signed integer interpretation is two's complement, which
/// routinely overflows (when interpreted as unsigned).  Signed overflow may only
/// occur if the inputs have the same sign, it is detected when the sign of the
/// result is opposite that of the inputs.
///
/// \todo The library is not explicit with regards to the fact that
/// operator+ is only useful with the unsigned interpretation.  A decision
/// must be made to either keep the library as is, or to promote full addition
/// to operator+, and the rationale for the decision
///
/// \todo What is the right place for this function?
/// It was added here because in practice multiplication overflows, as a draft
template<int NB, typename B>
constexpr ArithmeticResultTriplet<NB, B>
fullAddition(SWAR<NB, B> s1, SWAR<NB, B> s2) {
    using S = SWAR<NB, B>;
    constexpr auto
        SignBit = S{S::MostSignificantBit},
        LowerBits = SignBit - S{S::LeastSignificantBit};
    // prevent overflow by clearing the most significant bits
    auto
        s1prime = LowerBits & s1,
        s2prime = LowerBits & s2,
        resultPrime = s1prime + s2prime,
        s1Sign = SignBit & s1,
        s2Sign = SignBit & s2,
        signPrime = SignBit & resultPrime,
        result = resultPrime ^ s1Sign ^ s2Sign,
        // carry is set whenever at least two of the sign bits of s1, s2,
        // signPrime are set
        carry = (s1Sign & s2Sign) | (s1Sign & signPrime) | (s2Sign & signPrime),
        // overflow: the inputs have the same sign and different to result
        // same sign: s1Sign ^ s2Sign
        overflow = (s1Sign ^ s2Sign ^ SignBit) & (s1Sign ^ result);
    using BS = BooleanSWAR<NB, B>;
    return { result, BS{carry.value()}, BS{overflow.value()} };
};

template<int NB, typename B>
constexpr SWAR<NB, B>
saturatingUnsignedAddition(SWAR<NB, B> s1, SWAR<NB, B> s2) {
    const auto additionResult = fullAddition(s1, s2);
    // If we carry unsigned, we need to saturate: thus OR the carry bit with the
    // lane bits (carry because it happens to be earlier in the struct
    // declaration)
    return additionResult.carry.MSBtoLaneMask() | additionResult.result;
}

/// \brief Negation is useful only for the signed integer interpretation
template<int NB, typename B>
constexpr auto negate(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    constexpr auto Ones = S{S::LeastSignificantBit};
    return fullAddition(~input, Ones).result;
}

/// \brief Performs a generalized iterated application of an associative operator to a base
///
/// In algebra, the repeated application of an operator to a "base" has different names depending on the
/// operator, for example "a + a + a + ... + a" n-times would be called "repeated addition",
/// if * is numeric multiplication, "a * a * a * ... * a" n-times would be called "exponentiation of a to the n
/// power".
/// The general term in algebra is "iteration", hence the naming of this function.
/// Since * and "product" are frequently used in Algebra to denote the application of a general operator, we
/// keep the option to use the imprecise language of "product, base and exponent".  "Iteration" has a very
/// different meaning in programming and especially different in C++.
/// There may be iteration over an operator that is not associative (such as quaternion multiplication), this
/// function leverages the associative property of the operator to "halve" the count of iterations at each step.
/// \note There is a symmetrical operation to be implemented of associative iteration in the
/// "progressive" direction: instead of starting with the most significant bit of the count, down to the lsb,
/// and doing "op(result, base, count)"; going from lsb to msb doing "op(result, square, exponent)"
/// \tparam Operator a callable with three arguments: the left and right arguments to the operation
/// and the count to be used, the "count" is an artifact of this generalization
/// \tparam IterationCount loosely models the "exponent" in "exponentiation", however, it may not
/// be a number, the iteration count is part of the execution context to apply the operator
/// \param forSquaring is an artifact of this generalization
/// \param log2Count is to potentially reduce the number of iterations if the caller a-priori knows
/// there are fewer iterations than what the type of exponent would allow
template<
    typename Base, typename IterationCount, typename Operator,
    // the critical use of associativity is that it allows halving the
    // iteration count
    typename CountHalver
>
constexpr auto associativeOperatorIterated_regressive(
    Base base, Base neutral, IterationCount count, IterationCount forSquaring,
    Operator op, unsigned log2Count, CountHalver ch
) {
    auto result = neutral;
    if(!log2Count) { return result; }
    for(;;) {
        result = op(result, base, count);
        if(!--log2Count) { break; }
        result = op(result, result, forSquaring);
        count = ch(count);
    }
    return result;
}

template<int ActualBits, int NB, typename T>
constexpr auto multiplication_OverflowUnsafe_SpecificBitCount(
    SWAR<NB, T> multiplicand, SWAR<NB, T> multiplier
) {
    using S = SWAR<NB, T>;

    auto operation = [](auto left, auto right, auto counts) {
        auto addendums = makeLaneMaskFromMSB(counts);
        return left + (addendums & right);
    };

    auto halver = [](auto counts) {
        auto msbCleared = counts & ~S{S::MostSignificantBit};
        return S{msbCleared.value() << 1};
    };

    auto shifted = S{multiplier.value() << (NB - ActualBits)};
    return associativeOperatorIterated_regressive(
        multiplicand, S{0}, shifted, S{S::MostSignificantBit}, operation,
        ActualBits, halver
    );
}

/// \note Not removed yet because it is an example of "progressive" associative exponentiation
template<int ActualBits, int NB, typename T>
constexpr auto multiplication_OverflowUnsafe_SpecificBitCount_deprecated(
    SWAR<NB, T> multiplicand,
    SWAR<NB, T> multiplier
) {
    using S = SWAR<NB, T>;
    constexpr auto LeastBit = S::LeastSignificantBit;
    auto multiplicandDoubling = multiplicand.value();
    auto mplier = multiplier.value();
    auto product = S{0};
    for(auto count = ActualBits;;) {
        auto multiplicandDoublingMask = makeLaneMaskFromLSB(S{mplier});
        product = product + (multiplicandDoublingMask & S{multiplicandDoubling});
        if(!--count) { break; }
        multiplicandDoubling <<= 1;
        auto leastBitCleared = mplier & ~LeastBit;
        mplier = leastBitCleared >> 1;
    }
    return product;
}

// TODO(Jamie): Add tests from other PR.
template<int ActualBits, int NB, typename T>
constexpr auto exponentiation_OverflowUnsafe_SpecificBitCount(
    SWAR<NB, T> x,
    SWAR<NB, T> exponent
) {
    using S = SWAR<NB, T>;

    auto operation = [](auto left, auto right, auto counts) {
      const auto mask = makeLaneMaskFromMSB(counts);
      const auto product =
        multiplication_OverflowUnsafe_SpecificBitCount<ActualBits>(left, right);
      return (product & mask) | (left & ~mask);
    };

    // halver should work same as multiplication... i think...
    auto halver = [](auto counts) {
        auto msbCleared = counts & ~S{S::MostSignificantBit};
        return S{static_cast<T>(msbCleared.value() << 1)};
    };

    exponent = S{static_cast<T>(exponent.value() << (NB - ActualBits))};
    return associativeOperatorIterated_regressive(
        x,
        S{meta::BitmaskMaker<T, 1, NB>().value}, // neutral is lane wise..
        exponent,
        S{S::MostSignificantBit},
        operation,
        ActualBits,
        halver
    );
}

template<int NB, typename T>
constexpr auto multiplication_OverflowUnsafe(
    SWAR<NB, T> multiplicand,
    SWAR<NB, T> multiplier
) {
    return
        multiplication_OverflowUnsafe_SpecificBitCount<NB>(
            multiplicand, multiplier
        );
}

template<int NB, typename T>
struct SWAR_Pair{
    SWAR<NB, T> even, odd;
};

template<int NB, typename T>
constexpr SWAR<NB, T> doublingMask() {
    using S = SWAR<NB, T>;
    static_assert(0 == S::Lanes % 2, "Only even number of elements supported");
    using D = SWAR<NB * 2, T>;
    return S{(D::LeastSignificantBit << NB) - D::LeastSignificantBit};
}

template<int NB, typename T>
constexpr auto doublePrecision(SWAR<NB, T> input) {
    using S = SWAR<NB, T>;
    static_assert(
        0 == S::NSlots % 2,
        "Precision can only be doubled for SWARs of even element count"
    );
    using RV = SWAR<NB * 2, T>;
    constexpr auto DM = doublingMask<NB, T>();
    return SWAR_Pair<NB * 2, T>{
        RV{(input & DM).value()},
        RV{(input.value() >> NB) & DM.value()}
    };
}

template<int NB, typename T>
constexpr auto halvePrecision(SWAR<NB, T> even, SWAR<NB, T> odd) {
    using S = SWAR<NB, T>;
    static_assert(0 == NB % 2, "Only even lane-bitcounts supported");
    using RV = SWAR<NB/2, T>;
    constexpr auto HalvingMask = doublingMask<NB/2, T>();
    auto
        evenHalf = RV{even.value()} & HalvingMask,
        oddHalf = RV{(RV{odd.value()} & HalvingMask).value() << NB/2};
    return evenHalf | oddHalf;
}

}

#endif
