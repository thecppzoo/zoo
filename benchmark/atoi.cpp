#include "zoo/swar/SWAR.h"
#include "zoo/swar/associative_iteration.h"

#include <immintrin.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

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

// Note: eight digits can represent from 0 to (10^9) - 1, the logarithm base 2
// of 10^9 is slightly less than 30, thus, only 30 bits are needed.
auto lemire_as_zoo_swar(const char *chars) {
    uint64_t bytes;
    memcpy(&bytes, chars, 8);
    auto allCharacterZero = zoo::meta::BitmaskMaker<uint64_t, '0', 8>::value;
    using S8_64 = zoo::swar::SWAR<8, uint64_t>;
    S8_64 convertedToIntegers = S8_64{bytes - allCharacterZero};
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

namespace zoo {

std::size_t c_strLength(const char *s) {
    using S = swar::SWAR<8, std::size_t>;
    constexpr auto
        MSBs = S{S::MostSignificantBit},
        Ones = S{S::LeastSignificantBit};
    S bytes;
    for(auto base = s;; base += 8) {
        memcpy(&bytes.m_v, base, 8);
        auto firstNullTurnsOnMSB = bytes - Ones;
        // The first lane with a null will borrow and set its MSB on when
        // subtracted one.
        // The borrowing from the first null interferes with the subsequent
        // lanes, that's why we focus on the first null.
        // The lanes previous to the first null might keep their MSB on after
        // subtracting one (if their value is greater than 0x80).
        // This provides a way to detect the first null: It is the first lane
        // in firstNullTurnsOnMSB that "flipped on" its MSB
        auto cheapestInversionOfMSBs = ~bytes;
        auto firstMSBsOnIsFirstNull =
            firstNullTurnsOnMSB & cheapestInversionOfMSBs;
        auto onlyMSBs = zoo::swar::convertToBooleanSWAR(firstMSBsOnIsFirstNull);
        if(onlyMSBs) { // there is a null!
            auto firstNullIndex = onlyMSBs.lsbIndex();
            return firstNullIndex + (base - s);
        }
    }
}

std::size_t c_strLength_natural(const char *s) {
    using S = swar::SWAR<8, std::size_t>;
    S bytes;
    for(auto base = s;; base += 8) {
        memcpy(&bytes.m_v, base, 8);
        auto nulls = zoo::swar::equals(bytes, S{0});
        if(nulls) { // there is a null!
            auto firstNullIndex = nulls.lsbIndex();
            return firstNullIndex + (base - s);
        }
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

size_t avx2_strlen(const char* str) {
    const __m256i zero = _mm256_setzero_si256(); // Vector of 32 zero bytes
    size_t offset = 0;

    // Loop over the string in blocks of 32 bytes
    for (;; offset += 32) {
        // Load 32 bytes of the string into a __m256i vector
        __m256i data;// = _mm256_load_si256((const __m256i*)(str + offset));
        memcpy(&data, str + offset, 32);
        // Compare each byte with '\0'
        __m256i cmp = _mm256_cmpeq_epi8(data, zero);
        // Create a mask indicating which bytes are '\0'
        int mask = _mm256_movemask_epi8(cmp);

        // If mask is not zero, we found a '\0' byte
        if (mask) {
            // Calculate the index of the first '\0' byte using ctz (Count Trailing Zeros)
            return offset + __builtin_ctz(mask);
        }
    }
    // Unreachable, but included to avoid compiler warnings
    return offset;
}

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
