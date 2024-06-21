#include "atoi.h"
#include "atoi_impl.h"

#include "zoo/swar/associative_iteration.h"

#if ZOO_CONFIGURED_TO_USE_AVX()
#include <immintrin.h>
#endif

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <array>

static_assert(~uint32_t(0) == zoo::swar::SWAR<32, uint32_t>::LeastSignificantLaneMask);

// Copied from Daniel Lemire's GitHub at
// https://lemire.me/blog/2018/10/03/quickly-parsing-eight-digits/
// https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/ddb082981228f7256e9a4dbbf56fd4a335d78e30/2018/10/03/eightchartoi.c#L26C1-L34C2

uint32_t parse_eight_digits_swar(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, 8);
  val = val - 0x3030303030303030;
  uint64_t byte10plus   = ((val        * (1 + (0xa  <<  8))) >>  8) & 0x00FF00FF00FF00FF;
  uint64_t short100plus = ((byte10plus * (1 + (0x64 << 16))) >> 16) & 0x0000FFFF0000FFFF;
  short100plus *= (1 + (10000ULL << 32));
  return short100plus >> 32;
}

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

uint64_t calculateBase10(zoo::swar::SWAR<8, __uint128_t> convertedToIntegers) noexcept {
    auto by11base256 = convertedToIntegers.multiply(256*10 + 1);
    auto bytePairs = zoo::swar::doublePrecision(by11base256).odd;
    auto by101base2to16 = bytePairs.multiply(1 + (100 << 16));
    auto byteQuads = zoo::swar::doublePrecision(by101base2to16).odd;
    auto by10001base2to32 = byteQuads.multiply(1 + (10000ull << 32));
    // Now, truly work with 128 bits: combine two 32 bit results, each
    // corresponding to 8 bytes of inputs, into the the 64 bit result by
    // scaling one by 10^8
    auto byteOcts = zoo::swar::doublePrecision(by10001base2to32).odd;
    auto byHundredMillionBase2to64 =
        byteOcts.multiply(1 + (__uint128_t(100'000'000) << 64));
    return uint64_t(byHundredMillionBase2to64.value() >> 64);
}

// Note: eight digits can represent from 0 to (10^9) - 1, the logarithm base 2
// of 10^9 is slightly less than 30, thus, only 30 bits are needed.
uint32_t lemire_as_zoo_swar(const char *chars) noexcept {
    uint64_t bytes;
    memcpy(&bytes, chars, 8);
    auto allCharacterZero = zoo::meta::BitmaskMaker<uint64_t, '0', 8>::value;
    using S8_64 = zoo::swar::SWAR<8, uint64_t>;
    S8_64 convertedToIntegers = S8_64{bytes - allCharacterZero};
    auto rv = calculateBase10(convertedToIntegers);
    return rv;
}

std::size_t spaces_glibc(const char *ptr) {
    auto rv = 0;
    while(isspace(ptr[rv])) { ++rv; }
    return rv;
}

namespace zoo {

template<typename S>
std::size_t leadingSpacesCountAligned(S bytes) noexcept {
    /*
    space (0x20, ' ')
    form feed (0x0c, '\f')
    line feed (0x0a, '\n')
    carriage return (0x0d, '\r')
    horizontal tab (0x09, '\t')
    vertical tab (0x0b, '\v')*
    constexpr std::array<char, 6> SpaceCharacters = {
        0b10'0000, //0x20 space
        0b00'1101, // 0xD \r
        0b00'1100, // 0xC \f
        0b00'1011, // 0xB \v
        0b00'1010, // 0xA \n
        0b00'1001  //   9 \t
    },
    ExpressedAsEscapeCodes = { ' ', '\r', '\f', '\v', '\n', '\t' };
    static_assert(SpaceCharacters == ExpressedAsEscapeCodes); */
    static_assert(sizeof(S) == alignof(S));
    constexpr S Space{meta::BitmaskMaker<uint64_t, ' ', 8>::value};
    auto space = swar::equals(bytes, Space);
    auto belowEqualCarriageReturn = swar::constantIsGreaterEqual<'\r'>(bytes);
    auto belowTab = swar::constantIsGreaterEqual<'\t' - 1>(bytes);
    auto otherWhiteSpace = belowEqualCarriageReturn & ~belowTab;
    auto whiteSpace = space | otherWhiteSpace;
    auto notWhiteSpace = ~whiteSpace;
    auto rv = notWhiteSpace.value() ? notWhiteSpace.lsbIndex() : S::Lanes;
    return rv;
}

std::size_t leadingSpacesCount(const char *p) noexcept {
    using S = swar::SWAR<8, uint64_t>;
    S bytes;
    auto [base, misalignment] = blockAlignedLoad(p, &bytes.m_v);
    auto bitDisplacement = 8 * misalignment;

    // deal with misalignment setting the low part to spaces
    constexpr static S
        AllSpaces{meta::BitmaskMaker<uint64_t, ' ', 8>::value},
        AllOn = ~S{0};
    // blit the spaces in
    auto mask = S{AllOn.value() << bitDisplacement};
    auto misalignedEliminated = bytes & mask;
    auto spacesIntroduced = AllSpaces & ~mask;
    bytes = spacesIntroduced | misalignedEliminated;
    for(;;) {
        auto spacesThisBlock = leadingSpacesCountAligned(bytes);
        base += spacesThisBlock;
        if(8 != spacesThisBlock) { return base - p; }
        memcpy(&bytes.m_v, base, 8);
    }
}

auto leadingDigitsCount(const char *p) noexcept {
    using S = swar::SWAR<8, uint64_t>;
    S bytes;
    auto [base, misalignment] = blockAlignedLoad(p, &bytes.m_v);
    auto bitDisplacement = 8 * misalignment;
    constexpr static S
        AllZeroCharacter{meta::BitmaskMaker<uint64_t, '0', 8>::value},
        AllOn = ~S{0};
    // blit the zero-characters to the misaligned part
    auto mask = S{AllOn.value() << bitDisplacement};
    auto misalignedEliminated = bytes & mask;
    auto zeroCharactersIntroduced = AllZeroCharacter & ~mask;
    bytes = zeroCharactersIntroduced | misalignedEliminated;
    for(;;) {
        auto belowOrEqualTo9 = swar::constantIsGreaterEqual<'9'>(bytes);
        auto belowCharacter0 = swar::constantIsGreaterEqual<'0' - 1>(bytes);
        auto digits = belowOrEqualTo9 & ~belowCharacter0;
        auto nonDigits = ~digits;
        if(nonDigits) {
            auto nonDigitIndex = nonDigits.lsbIndex();
            return base + nonDigitIndex - p;
        }
        base += 8;
        memcpy(&bytes.m_v, base, 8);
    }
}

namespace impl {

template<typename> struct ConversionTraits;
template<> struct ConversionTraits<int32_t>{
    constexpr static auto NPositions = 9; // from 10^0 to 10^8
    using PowersOf10Array = std::array<int32_t, NPositions>;
    using DoublePrecision = uint64_t;
};
template<> struct ConversionTraits<int64_t>{
    constexpr static auto NPositions = 17; // from 10^0 to 10^16
    using PowersOf10Array = std::array<int64_t, NPositions>;
    using DoublePrecision = __uint128_t;
};

template<typename Result>
auto PowersOf10Array() {
    using Traits = ConversionTraits<Result>;
    typename Traits::PowersOf10Array rv{1};
    for (std::size_t i = 1; i < Traits::NPositions; ++i) {
        rv[i] = rv[i - 1] * 10;
    }
    return rv;
};

template<typename Return>
Return c_strToIntegral(const char *str) noexcept {
    auto LastFactor = PowersOf10Array<Return>();
    auto leadingSpaces = leadingSpacesCount(str);
    auto s = str + leadingSpaces;
    auto sign = 1;
    switch(*s) {
        case '-': sign = -1;
            [[fallthrough]];
        case '+': ++s; break;
        default: ;
    }

    using SWAR_BaseType = typename ConversionTraits<Return>::DoublePrecision;
    constexpr auto
        NBytes = sizeof(SWAR_BaseType),
        NBitsPerByte = 8ul; // 8 bits per byte
    using S = swar::SWAR<NBitsPerByte, SWAR_BaseType>;
    S bytes;
    auto [base, misalignment] = blockAlignedLoad(s, &bytes.m_v);
    auto bitDisplacement = NBitsPerByte * misalignment;
    constexpr static S
        AllZeroCharacter{meta::BitmaskMaker<SWAR_BaseType, '0', NBitsPerByte>::value},
        AllOn = ~S{0};

    auto mask = S{AllOn.value() << bitDisplacement};
    auto misalignedEliminated = bytes & mask;
    auto zeroCharactersIntroduced = AllZeroCharacter & ~mask;
    bytes = zeroCharactersIntroduced | misalignedEliminated;
    long accumulator = 0;

    for(;;) {
        auto belowOrEqualTo9 = swar::constantIsGreaterEqual<'9'>(bytes);
        auto belowCharacter0 = swar::constantIsGreaterEqual<'0' - 1>(bytes);
        auto digits = belowOrEqualTo9 & ~belowCharacter0;
        auto nonDigits = ~digits;
        if(nonDigits) {
            auto nonDigitIndex = nonDigits.lsbIndex();
            auto asIntegers = bytes - AllZeroCharacter; // upper lanes garbage
            auto integersInHighLanes =
                // split the shift in two steps because if nonDigitIndex is
                // zero, then you'd shift all bits, this would result in U.B.
                // for a single shift
                asIntegers.shiftLanesLeft(NBytes - 1 - nonDigitIndex)
                          .shiftLanesLeft(1);
            auto inBase10 = calculateBase10(integersInHighLanes);
            auto scaledAccumulator = accumulator * LastFactor[nonDigitIndex];
            return Return((scaledAccumulator + inBase10) * sign);
        }
        // all bytes are digits
        auto asIntegers = bytes - AllZeroCharacter;
        accumulator *= LastFactor.back();
        auto inBase10 = calculateBase10(asIntegers);
        accumulator += inBase10;
        base += NBytes;
        memcpy(&bytes.m_v, base, NBytes);
    }
}

}

int c_strToI(const char *str) noexcept {
    return impl::c_strToIntegral<int>(str);
}

int64_t c_strToL(const char *str) noexcept {
    return impl::c_strToIntegral<int64_t>(str);
}

/// \brief Helper function to fix the non-string part of block
template<typename S>
S adjustMisalignmentFor_strlen(S data, int misalignment) {
    // The speculative load has the valid data in the higher lanes.
    // To use the same algorithm as the rest of the implementation, simply
    // populate with ones the lower part, in that way there won't be nulls.
    constexpr typename S::type Zero{0};
    auto
        zeroesInMisalignedOnesInValid =
            (~Zero) // all ones
            <<  (misalignment * 8), // assumes 8 bits per char
        onesInMisalignedZeroesInValid = ~zeroesInMisalignedOnesInValid;
    return data | S{onesInMisalignedZeroesInValid};
}

std::size_t c_strLength(const char *s) {
    using S = swar::SWAR<8, uint64_t>;
    constexpr auto
        MSBs = S{S::MostSignificantBit},
        Ones = S{S::LeastSignificantBit};
    constexpr auto BytesPerIteration = sizeof(S::type);
    S initialBytes;

    auto indexOfFirstTrue = [](auto bs) { return bs.lsbIndex(); };

    // Misalignment must be taken into account because a SWAR read is
    // speculative, it might read bytes outside of the actual string.
    // It is safe to read within the page where the string occurs, and to
    // guarantee that, simply make aligned reads because the size of the SWAR
    // base size will always divide the memory page size
    auto [alignedBase, misalignment] = blockAlignedLoad(s, &initialBytes.m_v);
    auto bytes = adjustMisalignmentFor_strlen(initialBytes, misalignment);
    for(;;) {
        auto firstNullTurnsOnMSB = bytes - Ones;
        // The first lane with a null will borrow and set its MSB on when
        // subtracted one.
        // The borrowing from the first null interferes with the subsequent
        // lanes, that's why we focus on the first null.
        // The lanes previous to the first null might keep their MSB on after
        // subtracting one (if their value is greater than 0x80).
        // This provides a way to detect the first null: It is the first lane
        // in firstNullTurnsOnMSB that "flipped on" its MSB
        // According to The Art of Computer Programming, Knuth 4A pg 152,
        // credit due to Alan Mycroft
        auto cheapestInversionOfMSBs = ~bytes;
        auto firstMSBsOnIsFirstNull =
            firstNullTurnsOnMSB & cheapestInversionOfMSBs;
        auto onlyMSBs = swar::convertToBooleanSWAR(firstMSBsOnIsFirstNull);
        if(onlyMSBs) {
            return alignedBase + indexOfFirstTrue(onlyMSBs) - s;
        }
        alignedBase += BytesPerIteration;
        memcpy(&bytes, alignedBase, BytesPerIteration);
    }
}

std::size_t c_strLength_natural(const char *s) {
    using S = swar::SWAR<8, std::uint64_t>;
    S initialBytes;
    auto [base, misalignment] = blockAlignedLoad(s, &initialBytes.m_v);
    auto bytes = adjustMisalignmentFor_strlen(initialBytes, misalignment);
    for(;;) {
        auto nulls = zoo::swar::equals(bytes, S{0});
        if(nulls.value()) { // there is a null!
            auto firstNullIndex = nulls.lsbIndex();
            return firstNullIndex + base - s;
        }
        base += sizeof(S);
        memcpy(&bytes.m_v, base, 8);
    }
}

std::size_t c_strLength_manualComparison(const char *s) {
    using S = swar::SWAR<8, std::size_t>;
    S bytes;
    constexpr auto MSBs = S{S::MostSignificantBit};
    for(auto base = s;; base += 8) {
        memcpy(&bytes.m_v, base, 8);
        // A null byte is detected in two steps:
        // 1. it has the MSB off, and
        // 2. the least significant bits are also off.
        // The swar library allows the detection of lsbs off
        // By comparing greater equal to 0,
        // 0 can only be greater-equal to a byte with LSBs 0
        auto haveMSB_cleared = bytes ^ MSBs;
        auto lsbNulls = zoo::swar::greaterEqual_MSB_off(S{0}, bytes & ~MSBs);
        auto nulls = haveMSB_cleared & lsbNulls;
        if(nulls.value()) {
            auto firstNullIndex = nulls.lsbIndex();
            return firstNullIndex + (base - s);
        }
    }
}

#if ZOO_CONFIGURED_TO_USE_AVX()

/// \note Partially generated by Chat GPT 4
size_t avx2_strlen(const char* str) {
    const __m256i zero = _mm256_setzero_si256(); // Vector of 32 zero bytes
    size_t offset = 0;
    __m256i data;
    auto [alignedBase, misalignment] = blockAlignedLoad(str, &data);

    // AVX does not offer a practical way to generate a mask of all ones in
    // the least significant positions, thus we cant invoke adjustFor_strlen.
    // We will do the first iteration as a special case to explicitly take into
    // account misalignment
    auto maskOfMask = (~uint64_t(0)) << misalignment;

    auto compareAndMask =
        [&]() {
            // Compare each byte with '\0'
            __m256i cmp = _mm256_cmpeq_epi8(data, zero);
            // Create a mask indicating which bytes are '\0'
            return _mm256_movemask_epi8(cmp);
        };
    auto mask = compareAndMask();
    mask &= maskOfMask;

    // Loop over the string in blocks of 32 bytes
    for (;;) {
        // If mask is not zero, we found a '\0' byte
        if (mask) {
            // Calculate the index of the first '\0' byte counting trailing 0s
            auto nunNullByteCount = __builtin_ctz(mask);
            return alignedBase + offset + nunNullByteCount - str;
        }
        offset += 32;
        memcpy(&data, alignedBase + offset, 32);
        mask = compareAndMask();
    }
    // Unreachable, but included to avoid compiler warnings
    return offset;
}
#endif

}

/// \brief This is the last non-platform specific "generic" strlen in GLibC.
/// Taken from https://sourceware.org/git/?p=glibc.git;a=blob_plain;f=string/strlen.c;hb=6d7e8eda9b85b08f207a6dc6f187e94e4817270f
/// that dates to 2023-01-06 (a year ago at the time of writing)
std::size_t
STRLEN_old (const char *str)
{
  const char *char_ptr;
  const unsigned long int *longword_ptr;
  unsigned long int longword, himagic, lomagic;

  /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  for (char_ptr = str; ((unsigned long int) char_ptr
			& (sizeof (longword) - 1)) != 0;
       ++char_ptr)
    if (*char_ptr == '\0')
      return char_ptr - str;

  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to 8-byte longwords.  */

  longword_ptr = (unsigned long int *) char_ptr;

  /* Computing (longword - lomagic) sets the high bit of any corresponding
     byte that is either zero or greater than 0x80.  The latter case can be
     filtered out by computing (~longword & himagic).  The final result
     will always be non-zero if one of the bytes of longword is zero.  */
  himagic = 0x80808080L;
  lomagic = 0x01010101L;
  if (sizeof (longword) > 4)
    {
      /* 64-bit version of the magic.  */
      /* Do the shift in two steps to avoid a warning if long has 32 bits.  */
      himagic = ((himagic << 16) << 16) | himagic;
      lomagic = ((lomagic << 16) << 16) | lomagic;
    }
  if (sizeof (longword) > 8)
    abort ();

  /* Instead of the traditional loop which tests each character,
     we will test a longword at a time.  The tricky part is testing
     if *any of the four* bytes in the longword in question are zero.  */
  for (;;)
    {
      longword = *longword_ptr++;

      if (((longword - lomagic) & ~longword & himagic) != 0)
	{
	  /* Which of the bytes was the zero?  */

	  const char *cp = (const char *) (longword_ptr - 1);

	  if (cp[0] == 0)
	    return cp - str;
	  if (cp[1] == 0)
	    return cp - str + 1;
	  if (cp[2] == 0)
	    return cp - str + 2;
	  if (cp[3] == 0)
	    return cp - str + 3;
	  if (sizeof (longword) > 4)
	    {
	      if (cp[4] == 0)
		return cp - str + 4;
	      if (cp[5] == 0)
		return cp - str + 5;
	      if (cp[6] == 0)
		return cp - str + 6;
	      if (cp[7] == 0)
		return cp - str + 7;
	    }
	}
    }
}


#if ZOO_CONFIGURED_TO_USE_NEON()

#include <arm_neon.h>

namespace zoo {

/// \note uses the key technique of shifting by 4 and narrowing from 16 to 8 bit lanes in
/// aarch64/strlen.S at
/// https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/aarch64/strlen.S;h=ab2a576cdb5665e596b791299af3f4abecb73c0e;hb=HEAD
std::size_t neon_strlen(const char *str) {
    const uint8x16_t zero = vdupq_n_u8(0);
    size_t offset = 0;
    uint8x16_t data;
    auto [alignedBase, misalignment] = blockAlignedLoad(str, &data);

    auto compareAndConvertResultsToNibbles = [&]() {
        auto cmp = vceqq_u8(data, zero);
        // The result looks like, in hexadecimal digits, like this:
        // [ AA, BB, CC, DD, EE, FF, GG, HH, ... ] with each
        // variable A, B, ... either 0xF or 0x0.
        // instead of 16x8 bit results, we can see that as
        // 8 16 bit results like this
        // [ AABB, CCDD, EEFF, GGHH, ... ]
        // If we shift out a nibble from each element (shift right by 4):
        // [ ABB0, CDD0, EFF0, GHH0, ... ]
        // Narrowing from 16 to eight, we would get
        // [ AB, CD, EF, GH, ... ]
        auto straddle8bitLanePairAndNarrowToBytes = vshrn_n_u16(cmp, 4);
        return vget_lane_u64(vreinterpret_u64_u8(straddle8bitLanePairAndNarrowToBytes), 0);
    };
    auto nibbles = compareAndConvertResultsToNibbles();
    auto misalignmentNibbleMask = (~uint64_t(0)) << (misalignment * 4);
    nibbles &= misalignmentNibbleMask;
    for(;;) {
        if(nibbles) {
            auto trailingZeroBits = __builtin_ctz(nibbles);
            auto nonNullByteCount = trailingZeroBits / 4;
            return alignedBase + offset + nonNullByteCount - str;
        }
        alignedBase += sizeof(uint8x16_t);
        memcpy(&data, alignedBase, sizeof(uint8x16_t));
        nibbles = compareAndConvertResultsToNibbles();
    }
}

}
#endif
