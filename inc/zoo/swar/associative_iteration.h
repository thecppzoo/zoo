#ifndef ZOO_SWAR_ASSOCIATIVE_ITERATION_H
#define ZOO_SWAR_ASSOCIATIVE_ITERATION_H

#include "zoo/swar/SWAR.h"

namespace zoo::swar {

/// \note This code should be substituted by an application of "progressive" algebraic iteration
/// \note There is also parallelPrefix (to be implemented)
template<int NB, typename B>
constexpr SWAR<NB, B> parallelSuffix(SWAR<NB, B> input) {
    using S = SWAR<NB, B>;
    auto
        shiftClearingMask = S{~S::MostSignificantBit},
        doubling = input,
        result = S{0};
    auto
        bitsToXOR = NB,
        power = 1;
    for(;;) {
        if(1 & bitsToXOR) {
            result = result ^ doubling;
            doubling = doubling.shiftIntraLaneLeft(power, shiftClearingMask);
        }
        bitsToXOR >>= 1;
        if(!bitsToXOR) { break; }
        auto shifted = doubling.shiftIntraLaneLeft(power, shiftClearingMask);
        doubling = doubling ^ shifted;
        // 01...1
        // 001...1
        // 00001...1
        // 000000001...1
        shiftClearingMask =
            shiftClearingMask & S{shiftClearingMask.value() >> power};
        power <<= 1;
    }
    return S{result};
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

    multiplier = S{multiplier.value() << (NB - ActualBits)};
    return associativeOperatorIterated_regressive(
        multiplicand, S{0}, multiplier, S{S::MostSignificantBit}, operation,
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
